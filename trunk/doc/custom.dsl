<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY docbook.dsl PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA dsssl>
]>

<style-sheet>

<style-specification id="html" use="docbook">
<style-specification-body> 
    ;;
    ;;  Tweak a few options from the default HTML stylesheet
    ;;
    (define %html-ext% ".htm")
    (define %body-attr% '())
    (define %shade-verbatim% #t)
    (define %use-id-as-filename% #t)

    (define %graphic-default-extension% ".png")
    (define %admon-graphics% #t)
    (define ($admon-graphic$ #!optional (nd (current-node)))
      ;; REFENTRY admon-graphic
      ;; PURP Admonition graphic file
      ;; DESC
      ;; Given an admonition node, returns the name of the graphic that should
      ;; be used for that admonition.
      ;; /DESC
      ;; AUTHOR N/A
      ;; /REFENTRY
        (cond ((equal? (gi nd) (normalize "tip"))
	 (string-append %admon-graphics-path% "tip.png"))
	((equal? (gi nd) (normalize "note"))
	 (string-append %admon-graphics-path% "note.png"))
	((equal? (gi nd) (normalize "important"))
	 (string-append %admon-graphics-path% "important.png"))
	((equal? (gi nd) (normalize "caution"))
	 (string-append %admon-graphics-path% "caution.png"))
	((equal? (gi nd) (normalize "warning"))
	 (string-append %admon-graphics-path% "warning.png"))
	(else (error (string-append (gi nd) " is not an admonition.")))))

    (define %gentext-nav-tblwidth% "100%")

    ;; expect admon images in the same directory
    (define %admon-graphics-path% "")

    (define %generate-article-toc% #t)
    (define %generate-article-titlepage-on-separate-page% #t)
    (define %generate-article-toc-on-titlepage% #f)
</style-specification-body>
</style-specification>

<style-specification id="onehtml" use="html">
<style-specification-body> 
    ;;
    ;;  Same as above except all in one file
    ;;
    (define nochunks #t)
</style-specification-body>
</style-specification>

<external-specification id="docbook" document="docbook.dsl">

</style-sheet>
