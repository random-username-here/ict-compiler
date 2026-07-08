define miscdump
    set $errs_ptr = &misc::errs()
    print ($arg0)->dump(*$errs_ptr)
end

document miscdump
    Prints dumpable objects (objects with `void dump(std::ostream&) const`)
    Usefull for debugging AST trees
end
