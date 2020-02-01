Creating a glyph using spiros
=============================

Spiro is the work of `Raph Levien <https://levien.com/spiro/>`_, it
provides an alternate method of designing a glyph using a set of
on-curve points rather than the traditional mixture of on and off curve
points used for bezier splines.

-  G4 Curve Points (continuous up to the fourth derivative)
-  G2 Curve Points (continuous up to the second derivative)
   Basically this boils down to: If you have a sharp curve you are
   probably better off using a G2 point, while a more gentle curve would
   call for a G4.
-  Corner Points
-  Prev Constraint points (to be used when the contour changes from a
   curve to a straight line) |spiroprevconstraint|
-  Next Constraint points (to be used when the contour changes from a
   straight line to a curve) |spironextconstraint|

.. |spiroprevconstraint| image:: /images/spiroprevconstraint.png
.. |spironextconstraint| image:: /images/spironextconstraint.png


Step 1
******

As before let us try to edit the "C" glyph from Ambrosia.

.. figure:: /images/Cspiro0.png

Again we start with a blank glyph. Note that there is a button in the
shape of a spiral in the tool pane. If you press this button you go
into spiro mode, and the tools available to you change slightly.

(If you press the button again you go back to Bezier mode)

Again use File->Import to import a background image and then scale it
properly. (If you don't know how this is done, look at the
:ref:`previous page <editexample.Import>`)


Step 2
******

.. figure:: /images/Cspiro1.png

Select the G4 curve point (the tool on the left side of the third row).

G4 curve points have the nice properties that the slope of the splines
will be the same on either side of them, and the curvature of the splines
will be too.

Then move the pointer over the image and click to place a point at a point
on the edge of the bitmap image.

Step 3
******

.. figure:: /images/Cspiro2.png

Select the "next constraint" point which changes the contour nicely from a
curve to a straight line.

If you pick the wrong constraint (and I often do -- it will become obvious
later when the contour looks distorted here), then select the constraint
point and use :ref:`Element->Get Info <getinfo.Spiro>` to change the
point type -- or use the :doc:`Point menu </ui/menus/pointmenu>`.

Step 4
******

.. figure:: /images/Cspiro3.png

Now select the corner point from the tool menu (the one that looks like a square).

Place it at a location where the slope changes abruptly -- a corner.

We are now readly to talk about the "left tangent point". Pretend you are
standing on the corner point, facing toward the tangent point. Is the next
point after it (in this case the curved point) to your left or to your right?
If to your left, use a left tangen, if to your right, use a right tangent.

Step 5
******

.. figure:: /images/Cspiro4.png

Now we want to do some fiddly work on the top of the "C". Here we have a serif
with a slight curve to it between two corners, two abrupt changes of direction.

We need to get a close up view of the image in order to work more precisely, so
select the magnifying glass tool from the tool pane, move it to the middle of
the serif, and click it several times until the serif fills the screen.

Step 6
******

.. figure:: /images/Cspiro5.png

Generally a corner point should have a constraint (or another corner) point on
either side of it, so we need to pick another constraint. In this case the
contour will change from a straight line to a curve, so that means a "prev
constraint" point.

Step 7
******

.. figure:: /images/Cspiro6.png

Then proceeding to fill in the other points needed to make for a smooth curve
of the serif.

Step 8
******

.. figure:: /images/Cspiro6_5.png

And another smooth curve of the other side of the serif.

Step 9
******

.. figure:: /images/Cspiro7.png

Now it is no longer useful to have such a close view of the image, so grab the
magnifying glass tool again, and hold down the Alt (Meta, Option) key. The
cursor should change, and clicking it will zoom you out.

Then fill in the remainder of the points on this side.

Step 10
*******

.. figure:: /images/Cspiro8.png

As we approach the lower tip of the C we again need to zoom in

Step 11
*******

.. figure:: /images/Cspiro9.png

And eventually we have completed a rough outline of the glyph. Clicking on the
start point will close the curve.

Unfortunately the result isn't quite what we'd hoped. There are some rather
erratic bulges.

We can fix that by

    #. moving points around

       Use the pointer tool, click on a point (or hold down the shift key to
       select several points) and then drag them around.
    #. adding new points to the outline.

       Using the appropriate spiro tool, depress the mouse somewhere on the
       outline -- a new point appears there. You may now drag this point around.

.. figure:: /images/Cspirals.png

In the process of fixing things we can move a point so far that the spiro
converter can't make sense of it. All of a sudden our (almost) nice outline
turns into an erratic spirals.

Don't worry about it, just move the point back and things return to normal.
If you move the point too far things can get even worse and the outline will
disappear altogether. Don't worry about that either, just put the point back.
Or use Edit->Undo.

And enjoy the curious beauty of the spirals you have unintentionally created.

(Raph is working on this, and at some point we may lose the spirals entirely,
but they have a certain charm -- I'll be sorry to see them go)

.. figure:: /images/Cspiro10.png
