"""Build the 10 Nov progress-update deck on the UW Engineering template (python-pptx).

    python3 presentation/build_update_deck_2026-11-10.py TEMPLATE.pptx OUT.pptx ASSETS_DIR

TEMPLATE.pptx is engineering_powerpoint_template_16-9_widescreen.potx with its main part's content type
switched from presentationml.template to presentationml.presentation in [Content_Types].xml (python-pptx
refuses .potx otherwise). ASSETS_DIR holds hero.png (hardware/renders/Smart_Dumbbell_Concept_Render_v02.png),
layout.png (hardware/layout/layout-v0.2.png cropped to the drawing area), rig.png (ml/figures/recording-rig-v1.png)
and schematic.png (hardware/wiring/SCH_Smart Dumbbell_2026-11-08.pdf rendered and cropped). Fonts and colours come
from the template theme (Barlow Condensed headings, Georgia body, Engineering purple accents)."""