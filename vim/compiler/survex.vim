if exists("current_compiler")
  finish
endif
let current_compiler = "survex"

lchdir %:p:h
setlocal makeprg=cavern\ %
