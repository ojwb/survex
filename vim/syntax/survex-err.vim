syn match svxPErrSmall	/Error\s\+\d\.\d\{2\}%/
syn match svxPErrBig	/Error\s\+\d\{2,\}\.\d\{2\}%/
syn match svxErrSmall	/^\d\.\d*$/
syn match svxErrBig	/^\d\{2,\}\.\d*$/
syn match svxHVErrSmall	/\<[HV]: \d\.\d*\>/
syn match svxHVErrBig	/\<[HV]: \d\{2,\}\.\d*\>/
syn match svxPolygon	/^[-_.a-zA-Z0-9]\+\( [-=] [-_.a-zA-Z0-9]\+\)\+$/

hi link svxPErrSmall	Statement
hi link svxErrSmall	Statement
hi link svxHVErrSmall	Statement
hi link svxPErrBig	Error
hi link svxErrBig	Error
hi link svxHVErrBig	Error
hi link svxPolygon	Identifier

map <buffer> <F9> :quit<CR>
