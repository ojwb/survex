" Vim syntax file
" Language:     Survex
" Maintainer:   David Loeffler <dave@cucc.survex.com>
" Last Change:  2016-01-01
" Filenames:    *.svx
" URL:          [NONE]
" Note:         The definitions below are taken from the Survex user manual as of February 2005, for version 1.0.34; several inconsistencies discovered in the process were clarified by reference to source code.  Since updated for version 1.1.8.
"
" Copyright (C) 2005 David Loeffler
" Copyright (C) 2006,2016 Olly Betts
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

" Fudgery - this is used to mask out anything else from matching.
syn match svxAnything ".*" contained

" Command names: these first few take no interesting arguments
syn keyword svxCmd contained    alias begin cs date declination
syn keyword svxCmd contained    end entrance equate export
syn keyword svxCmd contained    include ref require
syn keyword svxCmd contained    solve title truncate

syn keyword svxCmdDeprecated contained  default prefix

" These commands accept the whole of the rest of the line as argument, irrespective of whitespace.
syn keyword svxCmd contained	copyright instrument team nextgroup=svxAnything

syn keyword svxCmd      calibrate sd units      contained nextgroup=svxQty skipwhite
syn keyword svxQty contained    altitude backbearing backclino backlength nextgroup=svxQty,svxUnit skipwhite
syn keyword svxQty contained    backcompass backgradient backtape bearing clino nextgroup=svxQty,svxUnit skipwhite
syn keyword svxQty contained    compass count counter declination nextgroup=svxQty,svxUnit skipwhite
syn keyword svxQty contained    default depth dx dy dz easting gradient nextgroup=svxQty,svxUnit skipwhite
syn keyword svxQty contained    length level northing plumb position nextgroup=svxQty,svxUnit skipwhite
syn keyword svxQty contained    tape nextgroup=svxQty,svxUnit skipwhite

syn keyword svxCmd      case    contained nextgroup=svxCase skipwhite
syn keyword svxCase contained   preserve toupper tolower contained

syn keyword svxCmd      data    contained nextgroup=svxStyle skipwhite
syn keyword svxStyle contained  default normal diving topofil nextgroup=svxField skipwhite
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

syn keyword svxCmd contained nextgroup=svxFlag skipwhite        flags
syn keyword svxFlag contained nextgroup=svxFlag skipwhite       not duplicate surface splay

syn keyword svxCmd contained nextgroup=svxInferrable skipwhite  infer
syn keyword svxInferrable contained nextgroup=svxOnOff skipwhite plumbs equates exports
syn keyword svxOnOff contained on off

syn keyword svxCmd contained nextgroup=svxVar,svxVarDeprecated skipwhite    set
syn keyword svxVar contained               blank comment decimal eol keyword minus
syn keyword svxVar contained               names omit plus separator
syn keyword svxVarDeprecated contained     root

syn keyword svxCmd contained nextgroup=svxQty skipwhite units
syn keyword svxUnit contained           yards feet metric metres meters
syn keyword svxUnit contained           deg degrees grads mils percent percentage
syn keyword svxUnit contained           deg degrees grads mils minutes

syn keyword svxCmd contained nextgroup=svxRef skipwhite fix
syn keyword svxRef contained		reference

" Miscellaneous things that are spotted everywhere
syn keyword svxMisc             - down up

" Comments
syn match svxComment ";.*"

" Strings (double-quote)
syn region svxString             start=+"+  end=+"+

" Catch errors caused by filenames containing whitespace
" This is just an example really, to show the kind of
" error-trapping that's possible
syn match svxFilenameError "\*include\s*[^"]\+\s\+[^\s"]\+"

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
   HiLink svxInferrable           Identifier
   HiLink svxOnOff                Special
   HiLink svxVar                  Identifier
   HiLink svxMisc                 Special
   HiLink svxAsterisk             Statement
   HiLink svxFilenameError        Error
   HiLink svxCmdDeprecated        Todo
   HiLink svxVarDeprecated        Todo
   delcommand HiLink
endif

let b:current_syntax = "survex"
