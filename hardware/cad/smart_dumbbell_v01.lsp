;;; Smart Dumbbell concept v0.1 - units mm - native AutoCAD solids.
;;; Run SD_BUILD only in a NEW EMPTY drawing. No user geometry is erased.
;;; Component envelopes are layout assumptions, not measured vendor dimensions.

(defun sd-layer (name color)
  (if (not (tblsearch "LAYER" name))
    (entmake (list '(0 . "LAYER") '(100 . "AcDbSymbolTableRecord")
                   '(100 . "AcDbLayerTableRecord") (cons 2 name)
                   '(70 . 0) (cons 62 color) '(6 . "Continuous"))))
  (setvar "CLAYER" name))

(defun sd-box (p q h / e)
  (command "_.BOX" p q h) (entlast))

(defun sd-cylx (x y z radius length / e)
  (command "_.CYLINDER" (list x y z) radius "_Axis" (list (+ x length) y z))
  (entlast))

(defun sd-cylz (x y z radius height)
  (command "_.CYLINDER" (list x y z) radius height) (entlast))

(defun sd-hex (x length radius / pts k a profile obj)
  (setq pts nil k 0)
  (repeat 6
    (setq a (* pi (/ (+ 30.0 (* k 60.0)) 180.0)))
    (setq pts (append pts (list (cons 10 (list (* radius (cos a)) (* radius (sin a)))))))
    (setq k (1+ k)))
  (entmake (append (list '(0 . "LWPOLYLINE") '(100 . "AcDbEntity")
    (cons 8 (getvar "CLAYER")) '(100 . "AcDbPolyline") '(90 . 6) '(70 . 1)) pts))
  (setq profile (entlast))
  (command "_.EXTRUDE" profile "" length)
  (setq obj (entlast))
  (command "_.ROTATE3D" obj "" "_Yaxis" '(0 0 0) 90)
  (command "_.MOVE" obj "" '(0 0 0) (list x 0 0))
  obj)

(defun sd-sub (base cutter)
  (command "_.SUBTRACT" base "" cutter "") base)

(defun sd-union (base other / ss)
  (command "_.UNION" base other "")
  (if (entget base) base (entlast)))

(defun sd-text (p height message)
  (entmake (list '(0 . "TEXT") '(100 . "AcDbEntity") (cons 8 (getvar "CLAYER"))
    '(100 . "AcDbText") (cons 10 p) (cons 40 height) (cons 1 message)
    '(50 . 0.0) '(7 . "Standard"))))

(defun sd-copy-shift (obj delta / before)
  (command "_.COPY" obj "" '(0 0 0) delta) (entlast))

(defun c:SD_BUILD (/ grip left right capL capR cut e all parts grp
                        shellset capset insetset i pair x yy zz xx
                        board battery oled screen sensor buttons lname lcolor
                        oldcm oldos olddyn)
  (if (ssget "_X")
    (princ "\nSD_BUILD requires a new empty drawing. No geometry changed.")
    (progn
      (setq oldcm (getvar "CMDECHO") oldos (getvar "OSMODE") olddyn (getvar "DYNMODE"))
      (setvar "CMDECHO" 0) (setvar "OSMODE" 0) (setvar "DYNMODE" 0)
      (setvar "INSUNITS" 4) (setvar "MEASUREMENT" 1) (setvar "LUNITS" 2)
      (setvar "LUPREC" 2) (setvar "DELOBJ" 1) (setvar "FACETRES" 5)
      (setvar "ISOLINES" 4) (setvar "UCSFOLLOW" 0)
      (command "_.UCS" "_World")
      (setq parts nil)

      ;; Hollow grip: 146 mm overall, 140 mm exposed, OD34 ID24.
      (sd-layer "01_GRIP" 250)
      (setq grip (sd-cylx -73 0 0 17 146))
      (sd-sub grip (sd-cylx -74 0 0 12 148))
      (sd-sub grip (sd-box '(-22 -7 10) '(2 7 10) 15))
      (setq parts (cons (list grip '(0 0 0)) parts))

      ;; Hexagonal heads: 100 across corners, 86.603 across flats.
      ;; End wall 3 mm, side wall 3 mm. Four pilot bosses per head.
      (sd-layer "02_HEAD_SHELLS" 8)
      (setq right (sd-hex 70 67 50))
      (sd-sub right (sd-hex 73 65 46.535898))
      (sd-sub right (sd-cylx 69 0 0 17.2 5))
      (setq left (sd-hex -137 67 50))
      (sd-sub left (sd-hex -138 65 46.535898))
      (sd-sub left (sd-cylx -74 0 0 17.2 5))
      (foreach yy '(-27 27)
        (foreach zz '(-27 27)
          (setq right (sd-union right (sd-cylx 127 yy zz 5 10)))
          (setq left (sd-union left (sd-cylx -137 yy zz 5 10)))))
      (foreach yy '(-27 27)
        (foreach zz '(-27 27)
          (sd-sub right (sd-cylx 126 yy zz 1.3 12))
          (sd-sub left (sd-cylx -138 yy zz 1.3 12))))
      ;; Top OLED opening and three button holes on electronics head.
      (sd-sub right (sd-box '(91 -13 38) '(123 13 38) 14))
      (foreach yy '(-12 0 12)
        (sd-sub right (sd-cylz 81 yy 38 3.2 15)))
      (setq parts (cons (list right '(0 0 0)) parts))
      (setq parts (cons (list left '(0 0 0)) parts))

      ;; Removable end plates with nominal M3 clearance holes.
      (sd-layer "03_END_CAPS" 4)
      (setq capL (sd-hex -140 3 50) capR (sd-hex 137 3 50))
      (foreach yy '(-27 27)
        (foreach zz '(-27 27)
          (sd-sub capL (sd-cylx -141 yy zz 1.7 5))
          (sd-sub capR (sd-cylx 136 yy zz 1.7 5))))
      (sd-sub capR (sd-box '(136 -6 -25) '(141 6 -25) 7))
      (setq parts (cons (list capL '(-60 0 0)) parts))
      (setq parts (cons (list capR '(60 0 0)) parts))

      ;; Assumed envelopes only; not exact vendor models.
      (sd-layer "04_BATTERY_ENVELOPE" 30)
      (setq battery (sd-box '(-128 -19 -12) '(-78 19 -12) 24))
      (setq parts (cons (list battery '(-30 0 75)) parts))
      (sd-layer "05_MCU_ENVELOPE" 3)
      (setq board (sd-box '(79 -9 -10) '(124 9 -10) 2))
      (setq parts (cons (list board '(0 0 65)) parts))
      (sd-layer "06_ELECTRONIC_DETAILS" 250)
      (setq e (sd-box '(99 -7 -8) '(117 7 -8) 3))
      (setq parts (cons (list e '(0 0 65)) parts))
      (setq e (sd-box '(130 -5 -24) '(137 5 -24) 5))
      (setq parts (cons (list e '(40 0 0)) parts))
      (sd-layer "07_OLED_BOARD" 3)
      (setq oled (sd-box '(89 -15 35) '(125 15 35) 2))
      (setq parts (cons (list oled '(0 0 90)) parts))
      (sd-layer "08_DISPLAY_GLASS" 5)
      (setq screen (sd-box '(92 -12 37) '(122 12 37) 6.5))
      (setq parts (cons (list screen '(0 0 96)) parts))
      (sd-layer "09_SENSOR_ENVELOPE" 1)
      (setq sensor (sd-box '(-20 -6 12) '(0 6 12) 6))
      (setq parts (cons (list sensor '(0 0 45)) parts))
      (sd-layer "10_ACCELEROMETER" 2)
      (setq e (sd-box '(74 -24 -25) '(99 -4 -25) 4))
      (setq parts (cons (list e '(0 0 40)) parts))
      (sd-layer "11_BUTTONS" 4)
      (foreach yy '(-12 0 12)
        (setq e (sd-cylz 81 yy 40 2.8 5))
        (setq parts (cons (list e '(0 0 65)) parts)))

      ;; Second assembly illustrates separable caps and component layout.
      (foreach pair parts
        (setq lname (cdr (assoc 8 (entget (car pair)))))
        (setq lcolor (cdr (assoc 62 (tblsearch "LAYER" lname))))
        (sd-layer (strcat "X_" lname) lcolor)
        (setq e (sd-copy-shift (car pair) (mapcar '+ '(0 220 0) (cadr pair))))
        (entmod (subst (cons 8 (strcat "X_" (cdr (assoc 8 (entget e)))))
          (assoc 8 (entget e)) (entget e))))

      (sd-layer "12_LABELS" 7)
      (sd-text '(-145 -83 0) 7 "SMART DUMBBELL / CONCEPT v0.1")
      (sd-text '(-145 -96 0) 4 "ASSEMBLED | 280 L x 100 W x 86.6 H mm")
      (sd-text '(-145 -106 0) 4 "Grip OD34 / ID24 | Shell 3 mm | Units: mm")
      (sd-text '(-145 -116 0) 4 "Layout study only - verify parts, retention and strength before fabrication")
      (sd-text '(-190 300 0) 7 "EXPLODED / INTERNAL LAYOUT")
      (sd-text '(-190 288 0) 4 "Orange: power | Green: boards | Blue: display | Red: PPG | Yellow: motion")
      (sd-text '(-190 277 0) 4 "All component envelopes provisional; no load-bearing or print approval")

      (setvar "CLAYER" "0")
      (command "_.UCSICON" "_OFF")
      (setvar "GRIDMODE" 0)
      (command "_.VPOINT" '(1 -1 0.8))
      (command "_.ZOOM" "_Extents")
      (command "_.VSCURRENT" "_Conceptual")
      (setvar "OSMODE" oldos) (setvar "DYNMODE" olddyn) (setvar "CMDECHO" oldcm)
      (princ (strcat "\nSD_BUILD COMPLETE. Native solids: " (itoa (sslength (ssget "_X" '((0 . "3DSOLID")))))))
      (princ "\nSave as Smart_Dumbbell_Concept_v01.dwg. Use layers to inspect individual parts.")
    ))
  (princ))

(princ "\nSmart Dumbbell v0.1 loaded. Run SD_BUILD in an empty drawing.")
(princ)
