" Vim syntax file
" Language:     Survex loop closure errors
"
" Copyright (C) 2006 Thomas Holder
"
" This program is free software; you can redistribute it and/or modify
" it under the terms of the GNU General Public License as published by
" the Free Software Foundation; either version 2 of the License, or
" (at your option) any later version.
"
" This program is distributed in the hope that it will be useful,
" but WITHOUT ANY WARRANTY; without even the implied warranty of
" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
" GNU General Public License for more details.
"
" You should have received a copy of the GNU General Public License
" along with this program; if not, write to the Free Software
" Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

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
