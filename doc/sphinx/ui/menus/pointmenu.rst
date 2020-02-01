The Point Menu
==============

This menu is only available in the Outline Character View.

.. _pointmenu.Curve:

.. object:: Curve

   If all the selected points are Curve points then this entry will be checked.
   Selecting it will change all selected points to be Curve points.

.. _pointmenu.Corner:

.. object:: Corner

   If all the selected points are Corner points then this entry will be checked.
   Selecting it will change all selected points to be Corner points.

.. _pointmenu.Tangent:

.. object:: Tangent

   If all the selected points are Tangent points then this entry will be
   checked. Selecting it will change all selected points to be Tangent points.

.. _pointmenu.Make-First:

.. object:: Make First

   If exactly one point is selected and it is on a closed path and it is not the
   "first point" of that path, then it will become the first point.

.. _pointmenu.Interpolated:

.. object:: Can Be Interpolated

.. _pointmenu.NotInterpolated:

.. object:: Can't Be Interpolated

   When editing truetype curves, an on-curve point can be omitted from the point
   list that is stored in the output font if it happens to be exactly midway
   between its two control points. This makes for slightly smaller files and is
   generally a good thing. However if the point is interpolated then it cannot
   be referred to in instructions (or reference point matching), so on rare
   occasions you will need to be able to turn off this interpolation.

.. _pointmenu.AddAnchor:

.. object:: Add Anchor Point...

   .. image:: /images/agetinfo.png
      :align: right

   This command adds a new :ref:`anchor point <overview.Anchors>` to the glyph.
   The last click in the window provides a default location for the point.

   See :ref:`the overview <overview.Anchors>` and the
   :ref:`Element->Get Info command <getinfo.Anchors>` for more info.

.. _pointmenu.Acceptable:

.. object:: Acceptable Extrema

   Tells the validator that it is OK for this spline to have extrema. You select
   a spline by selecting its two end-points.

.. _pointmenu.MkLine:

.. object:: Make Line

   If two adjacent points are selected then make the spline between them into a
   straight line. If two points are selected and they are the endpoints of their
   respective (open) contours, then join them with a line. If a point is
   selected and has no adjacent selected points then make its control points be
   on top of the point

.. _pointmenu.MakeArc:

.. object:: Make Arc

   If two adjacent points are selected then make the spline between them into an
   elliptical arc. If two points are selected and they are the endpoints of
   their respective (open) contours, then join them with an elliptical arc.

   FontForge chooses an ellipse which runs through the two points and is tangent
   to the slope at those points. This is not enough information to determine an
   ellipse (three points and two tangents are needed) so in general there will
   be an infinite number of solutions, thus FontForge may not pick the one you
   had your heart set on.

   It is also possible to specify a set of input constraints which cannot be met
   by any ellipse.

   FontForge will first search for ellipses so that one of the selected points
   lies one of the axes of the ellipse. If it can't find one like that it will
   perform a more general search.

   (If you hold down the <Alt> key when you select the menu item, then FontForge
   will place a copy of the full ellipse into the background layer -- this was
   for debugging, but I thought it kind of cool so I left it in).

.. _pointmenu.Insert:

.. object:: Insert Point on Spline at...

   Select the two end-points of a spline, then bring up this dialog. You can
   request that FontForge insert a point on the selected spline with either a
   given X or a given Y coordinate.

.. _pointmenu.CenterCP:

.. object:: Center Between Control Points

   In truetype, if an on-curve point is centered between its control points,
   then that point may be omitted when written to the output file. This command
   allows you to create such a point more easily.

.. _pointmenu.NameContour:

.. object:: Name Contour

   A contour may be named. This is designed for use in the Guide line (and
   Background) layer. You can attach a mnemonic name to a guide line (like
   "X-Height" or "Cap-Height" or whatever strikes your fancy).

.. _pointmenu.ClipPath:

.. object:: Make Clip Path

   Only meaningful in Type3 fonts. For a more complete description see the
   section on :ref:`Type3 editing <multilayer.ClipPath>`. The clipping path will
   be set to any selected contour(s) in the glyph. If no contour is selected
   then there will be no clipping path.
 