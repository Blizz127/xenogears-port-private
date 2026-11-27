# Each negative control must compile, link and fail its own state assertion.
reset_guard = "#ifdef XENO_PC_PORT\n    /* Retail 8001BAC4..BADC"
assert base.count(reset_guard) == 1
    "contiguous-volume-reset": base.replace(reset_guard, "#if 0\n    /* Retail 8001BAC4..BADC", 1),
    expected_failure = ("NAME TAIL FAIL native volume reset extent" if name == "contiguous-volume-reset"
                        else "NAME TAIL FAIL native GameState")
    rejected = rc != 0 and expected_failure in (out / f"{name}.log").read_text()
