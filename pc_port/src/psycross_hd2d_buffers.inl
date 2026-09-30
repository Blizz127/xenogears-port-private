/* HD-2D HOOK 4 producer: per-frame depth (OT slot) and coverage buffers.
 * Included by PsyCross src/render/PsyX_render.cpp (psycross_hd2d_buffers.patch,
 * sentinel _xeno_hd2d_buffers); the contract is in
 * pc_port/include_shim/hd2d_sprite.h (HOOK 4).
 *
 * Everything here runs only while a lane defines PcPort_Hd2dFrameBuffers:
 * every call site tests that weak symbol first (a link-time constant), so
 * without a lane no GL object is created, no GL state is touched and no
 * draw is issued.
 *
 * How: a second pass over the same batches. Each draw split is drawn once
 * as retail into the default framebuffer and, right after, once more with
 * the same program, vertices, uniforms, scissor, viewport, stencil and
 * depth state into an FBO that holds depth_tex (R16), coverage_tex (R8)
 * and its own depth/stencil buffer. With a lane linked, the four PSX
 * programs are compiled with two extra fragment outputs (locations 1 and
 * 2, fed by a flat per-vertex tag from a parallel VBO) that the default
 * framebuffer ignores (its only draw buffer is location 0), so the colour
 * pass is unchanged; the tag FBO routes location 0 to GL_NONE. Blending is
 * off in the tag pass, so every pixel holds the tag of the last primitive
 * whose fragment survived the same discards (texel 0, CLUT semi mode,
 * HOOK 3 alpha cut) and tests, in exact OT order. The tag FBO's depth and
 * stencil are cleared whenever the default framebuffer's are, so the mask
 * (DrawPrim stencil) tests agree too. */
#include "../include_shim/hd2d_sprite.h"

struct XenoHd2dTag { unsigned short depth, coverage; };

static XenoHd2dTag* g_hd2dTags;     /* one per g_vertexBuffer vertex */
static GLuint g_hd2dFbo, g_hd2dDepthTex, g_hd2dCoverageTex, g_hd2dDepthStencil, g_hd2dTagVbo;
static int g_hd2dW, g_hd2dH;
static int g_hd2dFailed;            /* setup failed (or GLES): flags stay 0 */
static int g_hd2dScene;             /* the current scene's tags are being produced */
static unsigned g_hd2dSlots;        /* OT entries walked in this scene */
static int g_hd2dOverride;          /* HOOK 3 took the primitive being parsed */

/* Sprite part packets (PsyX_Hd2dTagSpritePacket), two tables by present
 * parity: the OT built before a present is often walked after it. */
#define XENO_HD2D_SPRITE_SLOTS 2048
static uintptr_t g_hd2dSprites[2][XENO_HD2D_SPRITE_SLOTS];
static int g_hd2dSpriteCount[2];
static unsigned g_hd2dPresents;

static unsigned XenoHd2dHash(uintptr_t p)
{
	return (unsigned)((p >> 2) * 2654435761u) & (XENO_HD2D_SPRITE_SLOTS - 1);
}

extern "C" void PsyX_Hd2dTagSpritePacket(const void* packet)
{
	const uintptr_t key = (uintptr_t)packet & 0xFFFFFF;
	const int t = g_hd2dPresents & 1;
	unsigned h;
	if (!PcPort_Hd2dFrameBuffers || !key || g_hd2dSpriteCount[t] >= XENO_HD2D_SPRITE_SLOTS * 3 / 4)
		return;
	for (h = XenoHd2dHash(key); g_hd2dSprites[t][h]; h = (h + 1) & (XENO_HD2D_SPRITE_SLOTS - 1))
		if (g_hd2dSprites[t][h] == key)
			return;
	g_hd2dSprites[t][h] = key;
	g_hd2dSpriteCount[t]++;
}

static int XenoHd2dIsSprite(uintptr_t key)
{
	for (int t = 0; t < 2; t++)
	{
		if (!g_hd2dSpriteCount[t])
			continue;
		for (unsigned h = XenoHd2dHash(key); g_hd2dSprites[t][h]; h = (h + 1) & (XENO_HD2D_SPRITE_SLOTS - 1))
			if (g_hd2dSprites[t][h] == key)
				return 1;
	}
	return 0;
}

/* ---- tags (PsyX_GPU.cpp) -------------------------------------------------- */

void GR_Hd2dOtEntry(void)
{
	g_hd2dSlots++;
}

void GR_Hd2dPrimOverridden(void)
{
	g_hd2dOverride = 1;
}

/* Tag vertices [first, end) of the primitive with P_TAG code `code` in the
 * packet linked at `packet`. */
void GR_Hd2dTagPrim(int first, int end, int code, const void* packet)
{
	unsigned short coverage = 0, depth;
	const int overridden = g_hd2dOverride;
	g_hd2dOverride = 0;
	if (first >= end)
		return;
	if (!g_hd2dTags)
	{
		g_hd2dTags = (XenoHd2dTag*)calloc(MAX_VERTEX_BUFFER_SIZE, sizeof(XenoHd2dTag));
		if (!g_hd2dTags)
		{
			g_hd2dFailed = 1;
			return;
		}
	}
	switch (code & 0xE0)
	{
	case 0x20: coverage = (code & 4) ? PC_PORT_HD2D_COVER_TEXTURED : PC_PORT_HD2D_COVER_FLAT_2D; break;
	default: coverage = PC_PORT_HD2D_COVER_FLAT_2D; break;	/* lines, TILE, SPRT */
	}
	if (XenoHd2dIsSprite((uintptr_t)packet & 0xFFFFFF))
		coverage |= PC_PORT_HD2D_COVER_SPRITE;
	if (overridden)
		coverage |= PC_PORT_HD2D_COVER_OVERRIDE;
	depth = (unsigned short)(g_hd2dSlots == 0 ? 1 : g_hd2dSlots > 65535 ? 65535 : g_hd2dSlots);
	if (end > MAX_VERTEX_BUFFER_SIZE)
		end = MAX_VERTEX_BUFFER_SIZE;
	for (int v = first; v < end; v++)
	{
		g_hd2dTags[v].depth = depth;
		g_hd2dTags[v].coverage = coverage;
	}
}

/* ---- GL side (PsyX_render.cpp) ------------------------------------------ */

/* Shader sources for the lane build: the PSX programs gain a flat tag input
 * and two outputs. Returns the source with `inject` after "void main() {"
 * (in a static buffer per slot) or NULL. */
static const char* XenoHd2dInject(int slot, const char* source, const char* inject)
{
	static char* buffers[4];
	const char* at = strstr(source, "void main() {");
	size_t head, len;
	if (!at || slot < 0 || slot >= 4)
		return NULL;
	head = (size_t)(at - source) + strlen("void main() {");
	len = strlen(source) + strlen(inject) + 1;
	free(buffers[slot]);
	buffers[slot] = (char*)malloc(len);
	if (!buffers[slot])
		return NULL;
	memcpy(buffers[slot], source, head);
	strcpy(buffers[slot] + head, inject);
	strcat(buffers[slot], source + head);
	return buffers[slot];
}

static const char* const c_xenoHd2dVsDecl = "	in uvec2 a_hd2dTag; // _xeno_hd2d_buffers\n	flat out uvec2 v_hd2dTag;\n";
static const char* const c_xenoHd2dFsDecl = "	flat in uvec2 v_hd2dTag; // _xeno_hd2d_buffers\n	out vec4 xenoHd2dDepth;\n	out vec4 xenoHd2dCoverage;\n";
static const char* const c_xenoHd2dVsMain = "\n		v_hd2dTag = a_hd2dTag;\n";
static const char* const c_xenoHd2dFsMain =
	"\n		xenoHd2dDepth = vec4(float(v_hd2dTag.x) / 65535.0, 0.0, 0.0, 1.0);\n"
	"		xenoHd2dCoverage = vec4(float(v_hd2dTag.y) / 255.0, 0.0, 0.0, 1.0);\n";
#define XENO_HD2D_TAG_ATTRIB 5	/* after a_extra (4) */

/* GR_Shader_Compile, for a PSX program when the lane is linked: appends the
 * tag declarations to the (1 KiB) define strings, which precede the common
 * block and the shader bodies, and swaps in bodies that write the tag at the
 * top of main() (outputs written before a discard are discarded with it).
 * vs/fs are GR_Shader_Compile's PSX lists (body at [3] / [4]). */
static int XenoHd2dShaderSources(const char** vs, const char** fs, char* vsDefines, char* fsDefines)
{
#if defined(RENDERER_OGL) && !defined(ES2_SHADERS) && !defined(ES3_SHADERS)
	const char* v = XenoHd2dInject(0, vs[3], c_xenoHd2dVsMain);
	const char* f = XenoHd2dInject(1, fs[4], c_xenoHd2dFsMain);
	if (!v || !f)
		return 0;
	vs[3] = v;
	fs[4] = f;
	strcat(vsDefines, c_xenoHd2dVsDecl);
	strcat(fsDefines, c_xenoHd2dFsDecl);
	return 1;
#else
	(void)vs; (void)fs; (void)vsDefines; (void)fsDefines;
	g_hd2dFailed = 1;	/* GLES: no tag pass, HOOK 4 flags stay 0 */
	return 0;
#endif
}

static void XenoHd2dBindProgramLocations(GLuint program)
{
	glBindAttribLocation(program, a_extra, "a_extra");
	glBindAttribLocation(program, XENO_HD2D_TAG_ATTRIB, "a_hd2dTag");
	glBindFragDataLocation(program, 0, "fragColor");
	glBindFragDataLocation(program, 1, "xenoHd2dDepth");
	glBindFragDataLocation(program, 2, "xenoHd2dCoverage");
}

static void XenoHd2dDestroyTargets(void)
{
	if (g_hd2dFbo) glDeleteFramebuffers(1, &g_hd2dFbo);
	if (g_hd2dDepthTex) glDeleteTextures(1, &g_hd2dDepthTex);
	if (g_hd2dCoverageTex) glDeleteTextures(1, &g_hd2dCoverageTex);
	if (g_hd2dDepthStencil) glDeleteRenderbuffers(1, &g_hd2dDepthStencil);
	g_hd2dFbo = g_hd2dDepthTex = g_hd2dCoverageTex = g_hd2dDepthStencil = 0;
	g_hd2dW = g_hd2dH = 0;
}

static GLuint XenoHd2dTexture(GLenum internal, GLenum type, int w, int h)
{
	GLuint t = 0;
	glGenTextures(1, &t);
	glBindTexture(GL_TEXTURE_2D, t);
	glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, GL_RED, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return t;
}

/* (Re)create the targets at the drawable size. Restores the texture,
 * renderbuffer and framebuffer bindings. */
static int XenoHd2dTargets(int w, int h)
{
	GLint tex = 0, rb = 0, drawFb = 0, readFb = 0;
	if (g_hd2dFbo && w == g_hd2dW && h == g_hd2dH)
		return 1;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFb);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFb);
	XenoHd2dDestroyTargets();
	g_hd2dDepthTex = XenoHd2dTexture(GL_R16, GL_UNSIGNED_SHORT, w, h);
	g_hd2dCoverageTex = XenoHd2dTexture(GL_R8, GL_UNSIGNED_BYTE, w, h);
	glGenRenderbuffers(1, &g_hd2dDepthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, g_hd2dDepthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
	glGenFramebuffers(1, &g_hd2dFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, g_hd2dFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_hd2dDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_hd2dCoverageTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_hd2dDepthStencil);
	{
		/* fragment output 0 (colour) -> nothing, 1 -> depth, 2 -> coverage */
		const GLenum bufs[3] = { GL_NONE, GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(3, bufs);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
	}
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFb);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, readFb);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		eprinterr("[hd2d-buffers] tag framebuffer incomplete (0x%x); HOOK 4 flags stay 0\n", status);
		XenoHd2dDestroyTargets();
		g_hd2dFailed = 1;
		return 0;
	}
	g_hd2dW = w;
	g_hd2dH = h;
	eprintinfo("[hd2d-buffers] depth R16 + coverage R8 targets %dx%d\n", w, h);
	return 1;
}

/* Run body with the tag FBO as the draw framebuffer, then rebind `restore`. */
#define XENO_HD2D_WITH_FBO(restore, body) do { \
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_hd2dFbo); \
	body; \
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (restore)); \
} while (0)

/* GR_BeginScene, after the default framebuffer's depth/stencil clears: new
 * scene, all tags 0; depth/stencil cleared as the default framebuffer's
 * were (same scissor state). */
void GR_Hd2dSceneBegin(void)
{
	int w = 0, h = 0;
	GLint drawFb = 0;
	g_hd2dSlots = 0;
	g_hd2dOverride = 0;
	g_hd2dScene = 0;
	if (g_hd2dFailed)
		return;
	SDL_GL_GetDrawableSize(g_window, &w, &h);
	if (w <= 0 || h <= 0 || !XenoHd2dTargets(w, h))
		return;
	if (!g_hd2dTagVbo)
	{
		GLint vbo = 0;
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
		glGenBuffers(1, &g_hd2dTagVbo);
		glBindBuffer(GL_ARRAY_BUFFER, g_hd2dTagVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(XenoHd2dTag) * MAX_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFb);
	{
		const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
		const GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		XENO_HD2D_WITH_FBO(drawFb, {
			glClear(GL_DEPTH_BUFFER_BIT);
			glClear(GL_STENCIL_BUFFER_BIT);
			if (scissor) glDisable(GL_SCISSOR_TEST);
			glClearBufferfv(GL_COLOR, 1, zero);
			glClearBufferfv(GL_COLOR, 2, zero);
			if (scissor) glEnable(GL_SCISSOR_TEST);
		});
	}
	g_hd2dScene = 1;
}

/* GR_Clear (ClearImage, isbg): the fill is not a primitive: tags 0 in the
 * cleared area, depth cleared too, under the same scissor. */
void GR_Hd2dClear(void)
{
	GLint drawFb = 0;
	const GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLfloat depth = 1.0f;
	if (!g_hd2dScene)
		return;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFb);
	if (drawFb != 0)
		return;
	glGetFloatv(GL_DEPTH_CLEAR_VALUE, &depth);
	XENO_HD2D_WITH_FBO(drawFb, {
		glClearBufferfv(GL_COLOR, 1, zero);
		glClearBufferfv(GL_COLOR, 2, zero);
		glClearBufferfv(GL_DEPTH, 0, &depth);
	});
}

/* GR_MaterializeFramebufferRect (a VRAM copy into the frame): tags 0 in the
 * GL window rect [x0,x1) x [y0,y1). */
void GR_Hd2dZeroRect(int x0, int y0, int x1, int y1)
{
	GLint box[4], drawFb = 0;
	const GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLboolean scissor;
	if (!g_hd2dScene)
		return;
	if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
	if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
	if (x1 <= x0 || y1 <= y0)
		return;
	scissor = glIsEnabled(GL_SCISSOR_TEST);
	glGetIntegerv(GL_SCISSOR_BOX, box);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFb);
	glEnable(GL_SCISSOR_TEST);
	glScissor(x0, y0, x1 - x0, y1 - y0);
	XENO_HD2D_WITH_FBO(drawFb, {
		glClearBufferfv(GL_COLOR, 1, zero);
		glClearBufferfv(GL_COLOR, 2, zero);
	});
	glScissor(box[0], box[1], box[2], box[3]);
	if (!scissor)
		glDisable(GL_SCISSOR_TEST);
}

/* GR_UpdateVertexBuffer, after the vertex upload (the batch VAO is bound):
 * upload the tags and point the VAO's tag attribute at them. */
void GR_Hd2dUploadTags(int numVertices)
{
	GLint vbo = 0;
	if (!g_hd2dScene || !g_hd2dTags || !g_hd2dTagVbo || numVertices <= 0)
		return;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_hd2dTagVbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * sizeof(XenoHd2dTag), g_hd2dTags);
	glEnableVertexAttribArray(XENO_HD2D_TAG_ATTRIB);
	glVertexAttribIPointer(XENO_HD2D_TAG_ATTRIB, 2, GL_UNSIGNED_SHORT, sizeof(XenoHd2dTag), (const void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
}

/* DrawSplit, after the retail draw: the same triangles into the tag FBO
 * with blending off. Only on-screen draws (default framebuffer). */
void GR_Hd2dDrawSplitTags(int startVertex, int triangles)
{
	GLint drawFb = 0;
	GLboolean blend;
	if (!g_hd2dScene || triangles <= 0)
		return;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFb);
	if (drawFb != 0)
		return;
	blend = glIsEnabled(GL_BLEND);
	if (blend)
		glDisable(GL_BLEND);
	XENO_HD2D_WITH_FBO(drawFb, GR_DrawTriangles(startVertex, triangles));
	if (blend)
		glEnable(GL_BLEND);
}

/* Both present sites, just before HOOK 1. path 0 = scene, 1 = VRAM display;
 * vramShown: the scene present showed the VRAM display instead. */
void GR_Hd2dPresent(int path, int vramShown)
{
	PcPortHd2dBuffers b;
	int w = 0, h = 0;
	SDL_GL_GetDrawableSize(g_window, &w, &h);
	memset(&b, 0, sizeof b);
	b.depth_tex = g_hd2dDepthTex;
	b.coverage_tex = g_hd2dCoverageTex;
	b.width = g_hd2dW ? g_hd2dW : w;
	b.height = g_hd2dH ? g_hd2dH : h;
	if (path == 0 && g_hd2dScene && !vramShown && !g_hd2dFailed && g_hd2dW == w && g_hd2dH == h)
	{
		b.flags = PC_PORT_HD2D_BUFFERS_DEPTH | PC_PORT_HD2D_BUFFERS_COVERAGE;
		b.depth_max = g_hd2dSlots > 65535 ? 65535 : g_hd2dSlots;
	}
	PcPort_Hd2dFrameBuffers(&b);
	if (path == 0)
		g_hd2dScene = 0;
	/* sprite packets: keep this interval's and the previous one's */
	g_hd2dPresents++;
	memset(g_hd2dSprites[g_hd2dPresents & 1], 0, sizeof g_hd2dSprites[0]);
	g_hd2dSpriteCount[g_hd2dPresents & 1] = 0;
}
