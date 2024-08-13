" Vim syntax file
" Language:     Survex
" Maintainer:   David Loeffler <dave@cucc.survex.com>
" Last Change:  2024-08-09
" Filenames:    *.svx
" URL:          [NONE]
" Note:         This should be up to date for Survex 1.4.11
"
" Copyright (C) 2005 David Loeffler
" Copyright (C) 2006,2016,2017,2024 Olly Betts
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

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
   syntax clear
elseif exists("b:current_syntax")
   finish
endif

" Always ignore case
syn case ignore

" * introduces a command
syn match svxAsterisk "^\s*\*" nextgroup=SvxCmd,SvxCmdDeprecated skipwhite

" Other lines are data lines
syn match svxData ".*" contained nextgroup=svxDataTokens skipwhite

" Command names: these first few take no interesting arguments
syn keyword svxCmd contained    alias begin cs date declination
syn keyword svxCmd contained    end entrance equate export
syn keyword svxCmd contained    include ref require
syn keyword svxCmd contained    solve title truncate

syn keyword svxCmdDeprecated contained  default prefix

" These commands accept the whole of the rest of the line as argument, irrespective of whitespace.
syn keyword svxCmd contained	copyright instrument team nextgroup=svxAnything

syn keyword svxCmd      calibrate sd units      contained nextgroup=svxQty skipwhite
syn keyword svxQty contained    altitude backbearing backclino backlength nextgroup=svxQty,svxUnit,svxUnitDeprecated skipwhite
syn keyword svxQty contained    backcompass backgradient backtape bearing clino nextgroup=svxQty,svxUnit,svxUnitDeprecated skipwhite
syn keyword svxQty contained    compass count counter declination nextgroup=svxQty,svxUnit,svxUnitDeprecated skipwhite
syn keyword svxQty contained    default depth dx dy dz easting gradient nextgroup=svxQty,svxUnit,svxUnitDeprecated skipwhite
syn keyword svxQty contained    length level northing plumb position nextgroup=svxQty,svxUnit,svxUnitDeprecated skipwhite
syn keyword svxQty contained    left right up down nextgroup=svxQty,svxUnit,svxUnitDeprecated skipwhite
syn keyword svxQty contained    tape nextgroup=svxQty,svxUnit,svxUnitDeprecated skipwhite

syn keyword svxCmd      case    contained nextgroup=svxCase skipwhite
syn keyword svxCase contained   preserve toupper tolower contained

syn keyword svxCmd      data    contained nextgroup=svxStyle skipwhite
syn keyword svxStyle contained  default ignore
syn keyword svxStyle contained  normal diving topofil nextgroup=svxField skipwhite
syn keyword svxStyle contained  cartesian cylpolar nosurvey nextgroup=svxField skipwhite
syn keyword svxStyle contained  passage nextgroup=svxField skipwhite

syn keyword svxField contained nextgroup=svxField skipwhite     altitude backbearing backclino backlength
syn keyword svxField contained nextgroup=svxField skipwhite	backcompass backgradient backtape bearing clino
syn keyword svxField contained nextgroup=svxField skipwhite     compass count counter depth depthchange
syn keyword svxField contained nextgroup=svxField skipwhite     direction dx dy dz easting from
syn keyword svxField contained nextgroup=svxField skipwhite     fromcount fromdepth gradient ignore
syn keyword svxField contained nextgroup=svxField skipwhite     ignoreall length newline northing
syn keyword svxField contained nextgroup=svxField skipwhite     station tape to tocount todepth
syn keyword svxField contained nextgroup=svxField skipwhite     left right up down ceiling floor

syn keyword svxCmd contained nextgroup=svxFlag,svxNot skipwhite        flags
syn keyword svxFlag contained nextgroup=svxFlag,svxNot skipwhite       duplicate surface splay
syn keyword svxNot contained nextgroup=svxFlag,svxBadNot skipwhite     not
syn keyword svxBadNot contained nextgroup=svxFlag,svxBadNot skipwhite  not

syn keyword svxCmd contained nextgroup=svxInferrable skipwhite  infer
syn keyword svxInferrable contained nextgroup=svxOnOff skipwhite plumbs equates exports
syn keyword svxOnOff contained on off
syn keyword svxCmd contained nextgroup=svxNorthType skipwhite  cartesian
syn keyword svxNorthType contained skipwhite grid magnetic true

syn keyword svxCmd contained nextgroup=svxVar,svxVarDeprecated skipwhite    set
syn keyword svxVar contained               blank comment decimal eol keyword minus
syn keyword svxVar contained               names omit plus separator
syn keyword svxVarDeprecated contained     root

syn keyword svxCmd contained nextgroup=svxQty skipwhite units
syn keyword svxUnit contained           yards feet metric metres meters
syn keyword svxUnit contained           degs degrees grads minutes
syn keyword svxUnit contained           percent percentage
syn keyword svxUnitDeprecated contained mils

syn keyword svxCmd contained skipwhite fix
"FIXME: This is wrong - it's "*fix <station> reference ..."
"syn keyword svxCmd contained nextgroup=svxRef skipwhite fix
"syn keyword svxRef contained		reference

" Miscellaneous things that are highlighted in data lines
syn match svxDataTokens "\<\(d\|u\|down\|up\|level\)\>\|[-+]v\>\|\(\s\|^\)\@<=\(-\|\.\{1,3}\|[-+]v\)\(\s\|;\|$\)\@="

" Comments
syn match svxComment ";.*"

" Strings (double-quote)
syn region svxString             start=+"+  end=+"+

" Catch errors caused by filenames containing whitespace
" This is just an example really, to show the kind of
" error-trapping that's possible
syn match svxFilenameError "\*\s*include\>\s*[^\s";]\+\s\+[^";]\+"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_survex_syn_inits")
   if version < 508
     let did_survex_syn_inits = 1
     command -nargs=+ HiLink hi link <args>
   else
     command -nargs=+ HiLink hi def link <args>
   endif

   HiLink svxString               String
   HiLink svxComment              Comment
   HiLink svxCmd                  Statement
   HiLink svxStyle                Type
   HiLink svxUnit                 Identifier
   HiLink svxQty                  Identifier
   HiLink svxCase                 Identifier
   HiLink svxField                Identifier
   HiLink svxFlag                 Identifier
   HiLink svxNot                  Identifier
   HiLink svxBadNot               Error
   HiLink svxInferrable           Identifier
   HiLink svxOnOff                Special
   HiLink svxVar                  Identifier
   HiLink svxDataTokens           Special
   HiLink svxAsterisk             Statement
   HiLink svxFilenameError        Error
   HiLink svxCmdDeprecated        Todo
   HiLink svxUnitDeprecated       Todo
   HiLink svxVarDeprecated        Todo
   delcommand HiLink
endif

let b:current_syntax = "survex"
