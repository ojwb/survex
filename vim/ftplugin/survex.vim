if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal tabstop=8
compiler survex

map <buffer> <F5> :!aven %<.3d<CR>
map <buffer> <F7> :write<CR>:make<CR>
map <buffer> <F9> :sview %<.err<CR>
