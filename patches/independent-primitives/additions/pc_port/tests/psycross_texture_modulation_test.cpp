static bool runObliqueQuadCoverage() {
    using Point = std::array<int,2>;
    using Quad = std::array<Point,4>;
    // An interior shape, a clipped shape with integer points along its
    // edges, one projected across the screen and one with a degenerate half.
    const Quad shapes[] = {
        {{{5,9},{39,19},{17,45},{58,28}}},
        {{{-44,66},{-8,6},{91,39},{37,-10}}},
        {{{-310,71},{523,-87},{-145,271},{-45,133}}},
        {{{12,5},{19,16},{26,27},{-3,48}}}
    };
    POLY_F4 background{}, marker{};
    background.code = marker.code = 0x28;
    background.b0 = marker.g0 = 248;
    background.x0 = background.x2 = background.y0 = background.y1 = 3;
    background.x1 = background.x3 = background.y2 = background.y3 = 60;
    marker.x0 = marker.x2 = marker.y0 = marker.y1 = 21;
    marker.x1 = marker.x3 = marker.y2 = marker.y3 = 25;
    for (const Quad& shape : shapes) {
        for (int mirror = 0; mirror < 4; ++mirror) {
            Quad v = shape;
            for (Point& pt : v) {
                if (mirror & 1) pt[0] = size - 1 - pt[0];
                if (mirror & 2) pt[1] = size - 1 - pt[1];
            }
            POLY_F4 quad{};
            quad.code = 0x28;
            quad.r0 = 248;
            setXY4(&quad, v[0][0],v[0][1], v[1][0],v[1][1],
                          v[2][0],v[2][1], v[3][0],v[3][1]);
            clearScene(0,0,0);
            ClearSplits();
            GLint before[4], after[4];
            glGetIntegerv(GL_VIEWPORT, before);
            if (!decode(reinterpret_cast<P_TAG*>(&background)) ||
                !decode(reinterpret_cast<P_TAG*>(&quad)) ||
                !decode(reinterpret_cast<P_TAG*>(&marker)) ||
                g_vertexIndex != 18 || g_splitIndex != 1 || !flushPackets())
                return false;
            glGetIntegerv(GL_VIEWPORT, after);
            if (!std::equal(before,before+4,after)) return false;
            Image frame;
            glReadPixels(0,0,size,size,GL_RGBA,GL_UNSIGNED_BYTE,frame.data());
            for (int y=0; y<size; ++y) for (int x=0; x<size; ++x) {
                int expected = x>=3 && x<60 && y>=3 && y<60 ? 0x7c00 : 0;
                if (covered({v[0],v[1],v[2]},x,y) || covered({v[1],v[2],v[3]},x,y))
                    expected = 0x001f;
                if (x>=21 && x<25 && y>=21 && y<25) expected = 0x03e0;
                const int actual = rgb555(frame[(size-1-y)*size+x]);
                if (actual != expected) {
                    fprintf(stderr,"Opaque quad %td mirror %d at (%d,%d): %04x, expected %04x\n",
                            &shape-shapes,mirror,x,y,actual,expected);
                    return false;
                }
            }
        }
    }
    puts("PASS opaque oblique quads, clipping, mixed draw order and viewport restore");
    return true;
}

        ok = runFieldCoverage() && runRawQuadOverlap() && runObliqueQuadCoverage();
