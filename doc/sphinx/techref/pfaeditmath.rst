FontForge's math
================

Being a brief description of the mathematics underlying various of FontForge's
commands

It is presumed that you understand about parameterized splines, if not look at
the description of :doc:`Bézier curves <bezier>`.

* :ref:`Linear Transformations <pfaeditmath.Linear>`
* :ref:`Finding maxima and minima of a spline <pfaeditmath.maxima>`
* :ref:`Rasterizing a Glyph <pfaeditmath.Rasterizing>`

  * finding intersections
  * removing overlap
* :ref:`Approximating a spline <pfaeditmath.Approximating>`
* :ref:`Stroking a spline (historical) <pfaeditmath.Stroke>`
* :ref:`Approximating a cubic spline by a series of quadratic splines <bezier.ps2ttf>`

--------------------------------------------------------------------------------


.. _pfaeditmath.Linear:

Linear Transformations
----------------------

A linear transformation is one where the spline described by transforming the
end and control points will match the transformed spline. This includes most of
the common transformations you might wish:

.. rubric:: translation

.. math::

   x' &= x + \mathrm{d} x \\
   y' &= y + \mathrm{d} y

.. rubric:: scaling

.. math::

   x' &= s_x x \\
   y' &= s_y y

.. rubric:: rotation

.. math::

   x' &= \cos(A) x + \sin(A) y \\
   y' &= -\sin(A) x + \cos(A) y

.. rubric:: skewing

.. math::

   x' &= x + \sin(A) y \\
   y' &= y


--------------------------------------------------------------------------------


.. _pfaeditmath.maxima:

Finding maxima and minima of a spline
-------------------------------------

The maximum or minimum of a spline (along either the x or y axes) may be found
by taking the first derivative of that spline with respect to t. So if we have a
spline

.. math::

   x &= a_x t^3 + b_x t^2 + c_x t + d_x \\
   y &= a_y t^3 + b_y t^2 + c_y t + d_y

and we wish to find the maximum point with respect to the x axis we set:

.. math::

   \frac{\mathrm{d}x}{\mathrm{d}y} &= 0 \\
   3 a_x t^2 + 2 b-x t + c_x &= 0

and then using the quadratic formula we can solve for t:

.. math::

   t = \frac{-2 b_x \pm \sqrt{4b_x^2 - 4 \cdot 3a_x c_x}}{2 \cdot 3a_x}

--------------------------------------------------------------------------------


.. _pfaeditmath.POI:

Finding points of inflection of a spline
----------------------------------------

A point of inflection occurs when :math:`\frac{\mathrm{d}^2y}{\mathrm{d}x^2} = 0`
(or infinity).

Unfortunately this does not mean that :math:`\frac{\mathrm{d}^2y}{\mathrm{d}t^2} = 0`
or :math:`\frac{\mathrm{d}^2x}{\mathrm{d}t^2} = 0`.

..
   TODO: Someone verify this? Seems a bit iffy

.. math::

   \frac{\mathrm{d}^2y}{\mathrm{d}x^2} &=
   \frac{
         \frac{\mathrm{d}}{\mathrm{d}t}
         \left(
            \frac
               {\frac{\mathrm{d}y}{\mathrm{d}t}}
               {\frac{\mathrm{d}x}{\mathrm{d}t}}
         \right)
      }{\frac{\mathrm{d}x}{\mathrm{d}t}} \\
   &
   \frac{
      \frac{\mathrm{d}x}{\mathrm{d}t} \frac{\mathrm{d}^2y}{\mathrm{d}t^2} -
      \frac{\mathrm{d}y}{\mathrm{d}t} \frac{\mathrm{d}^2x}{\mathrm{d}t^2}
   }{\left( \frac{\mathrm{d}x}{\mathrm{d}t} \right)^3}

After a lot of algebra this boils down to the quadratic in t:

.. math::

   3(a_x b_y - a_y b_x)t^2 + 3(c_x a_y - c_y a_x)t + c_x b_y - c_y b_x = 0

If you examine this closely you will note that a quadratic spline
(:math:`a_y = a_x = 0`) can never have a point of inflection.

--------------------------------------------------------------------------------


.. _pfaeditmath.Rasterizing:

Rasterizing a glyph
-------------------


.. _pfaeditmath.Approximating:

Approximating a spline
----------------------

Many of FontForge's commands need to fit a spline to a series of points. The
most obvious of these are the :menuselection:`Edit --> Merge`, and
:menuselection:`Element --> Simplify` commands, but many others rely on the same
technique. Let us consider the case of the Merge command, suppose we have the
following splines and we wish to remove the middle point and generate a new
spline that approximates the original two:

|mergepre| --> |mergepost|

.. |mergepre| image:: /images/mergepre.png
.. |mergepost| image:: /images/mergepost.png

FontForge uses a least squares approximation to determine the new spline. It
calculates the locations of several points along the old splines, and then it
guesses [#f1]_ at t values for those points.

Now a cubic :doc:`Bézier <bezier>` spline is determined by its two end points
(:math:`P_0` and :math:`P_1`) and two control points (:math:`CP_0` and :math:`CP_1`),
which specify the slope at those end points). Here we know the end points, so
all we need is to find the control points. The spline can also be expressed as a
cubic polynomial:

.. math:: S(t) = a t^3 + b t^2 + c t + d

with

.. math::

   d &= P_0 \\
   c &= 3 CP_0 - 3 P_0 \\
   b &= 3 CP_1 - 6 CP_0 + 3 P_0 \\
   a &= P_1 - 3 CP_1 + 3 CP_0 - P_0

substituting

.. math::

   S(t) =& (P_1 - 3CP_1 + 3CP_0 - P_0) t^3 + \\
      & (3CP_1 - 6CP_0 + 3P_0) t^2 + \\
      & (3CP_0 - 3P_0) t + \\
      & P_0

rearranging

.. math::

   S(t) &= (-3t^3 + 3t^2) CP_1 + \\
      & (3t^3 - 6t^2 + 3t) CP_0 + \\
      & (P_1 - P_0)t^3 + 3P_0 t^2 - 3P_0 t + P_0

Now we want to minimize the sum of the squares of the difference between the
value we approximate with the new spline, :math:`S(t_i)`, and the actual value we
were given, :math:`P_i`.

.. math:: \sum [ S(t_i) - P_i ]^2

Now we have four unknown variables, the x and y coordinates of the two control
points. To find the minimum of the above error term we must take the derivative
with respect to each variable, and set that to zero. Then we have four equations
with four unknowns and we can solve for each variable.

.. math::

   \sum 2 ( -3t^3 + 3t^2 ) [ S_x(t_i) - P_{i_x} ] = 0 \\
   \sum 2 ( -3t^3 + 3t^2 ) [ S_y(t_i) - P_{i_y} ] = 0 \\
   \sum 2 ( 3t^3 - 6t^2  + 3t ) [ S_x(t_i) - P_{i_x} ] = 0 \\
   \sum 2 ( 3t^3 - 6t^2  + 3t ) [ S_y(t_i) - P_{i_y} ] = 0 

Happily for us, the x and y terms do not interact and my be solved separately.
The procedure is the same for each coordinate, so I shall only show one:

.. math::

   \sum 2 ( -3t^3 + 3t^2 ) [ S_x(t_i) - P_{i_x} ] = 0 \\
   \sum 2 ( 3t^3 - 6t^2  + 3t ) [ S_x(t_i) - P_{i_x} ] = 0\\
   \Rightarrow

.. math::

   \sum \left[
      (-3t^3 + 3t^2)CP_{1_x} + \\
      (-3t^3 + 3t^2)(3t^3 - 6t^2 + 3t)CP_{0_x} + \\
      (P_{1_x} - P_{0_x})t^3 + 3P_{0_x}t^2 - 3P_{0_x}t + P_{0_x} - P_{i_x}
   \right] = 0

.. math::

   CP_{1_x} \sum ( -3t^3 + 3t^2 ) ( -3t^3 + 3t^2 ) = \\
   -\sum ( -3t^3 + 3t^2 )
   [
      ( 3t^3 - 6t^2 + 3t ) CP_{0_x} + \\
      ( P_{1_x} - P_{0_x} ) t^3 + 3P_{0_x}t^2 - 3P_{0_x}t + P_{0_x} - P_{i_x}
   ]

.. math::

   CP_{1_x} = -\frac{
      \sum ( -3t^3 + 3t^2 )
      [
         ( 3t^3 - 6t^2 + 3t ) CP_{0_x} + \\
         ( P_{1_x} - P_{0_x} ) t^3 + 3P_{0_x}t^2 - 3P_{0_x}t + P_{0_x} - P_{i_x}
      ]
   }{
      \sum ( -3t^3 + 3t^2 ) ( -3t^3 + 3t^2 )
   }

Now this can be plugged into the other equation

.. math::

   \sum ( 3t^3 - 6t^2 + 3t )
   [
      ( -3t^3 + 3t^2 ) CP_{1_x} + \\
      ( 3t^3 - 6t^2 + 3t ) CP_{0_x} + \\
      ( P_{1_x} - P_{0_x} ) t^3 + 3P_{0_x}t^2 - 3P_{0_x}t + P_{0_x} - P_{i_x}
   ]
   = 0

And we can solve for :math:`CP_{0_x}` and then :math:`CP_{1_x}`. The algebra
becomes very messy, with lots of terms, but the concepts are simple.

Thus we know where the control points are. We know where the end points are. We
have our spline.


Why that didn't (quite) work
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The above matrix yields a curve which is a good approximation to the original
two. But it has one flaw: There is no constraint placed on the slopes, and
(surprisingly) the slopes at the end-points of the above method are not close
enough to those of the original, and the human eye can now detect the join
between this generated spline and the two that connect to it.

Generally we will know the slopes at the end points as well as the end points
themselves.

Let's try another approach, based on better geometry. Givens:

* the start point
* the slope at the start point
* the end point
* the slope at the end point

We want to find the two control points. Now it may seem that specifying the
slope specifies the control point but this is not so, it merely specifies the
direction in which the control point lies. The control point may be anywhere
along the line through the start point in that direction, and each position will
give a different curve.

So we can express the control point by saying it is :math:`CP_0 = P_0 + r_0 \Delta_0`
where :math:`\Delta_0` is a vector in the direction of the slope, and :math:`r_0`
is the distance in that direction. Similarly for the end point:
:math:`CP_1 = P_1 + r_1 \Delta_1`

We want to find :math:`r_0` and :math:`r_1`.

Converting from bezier control points into a polynomial gives us

.. math::

   S(t) &= at^3 + bt^2 + ct + d \\
   d &= P_0 \\
   c &= 3(CP_0 - P_0) \\
   b &= 3(CP_1 - CP_0) - c \\
   a &= P_1 - P_0 - c -b

Substituting we get

.. math::
   d &= P_0 \\
   c &= 3r_0 \Delta_0 \\
   b &= 3( P_1 - P_0 + r_1 \Delta_1 - 2r_0 \Delta_0 ) \\
   a &= 2( P_0 - P_1 ) + 3 ( r_0 \Delta_0 - r_1 \Delta_1 )

For least squares we want to minimize :math:`\sum(S(t_i) - P_i)^2`. Taking
derivatives with both :math:`r_0` and :math:`r_1`, we get:

.. math::

   \sum 2(3t^3 - 6t^2 + 3t)\Delta_0[S(t_i) - P_i] &= 0 \\
   \sum 2(-3t^3 + 3t^2)\Delta_1[S(t_i) - P_i] &= 0

dividing by constants and substituting, we get

.. math::

   \sum (3t^3 - 6t^2 + 3t) [
      P_0 - P_i + 3(P_1 - P_0)t^2 + \\
      2(P_0-P_1)t^3 + \\
      \Delta_0(3t - 6t^2 + 3t^3)r_0 + \\
      \Delta_1(3t^2 - 3t^3)r_1
   ] = 0

.. math::

   \sum (-3t^3 + 3t^2) [
      P_0 - P_i + 3(P_1 - P_0)t^2 + \\
      2(P_0 - P_1)t^3 + \\
      \Delta_0(3t - 6t^2 + 3t^3)r_0 + \\
      \Delta_1(3t^2 - 3t^3)r_1
   ] = 0

Again we have two linear equations in two unknowns (:math:`r_0`, and
:math:`r_1`), and (after a lot of algebraic grubbing) we can solve for them.
One interesting thing here is that the x and y terms do not separate but must
be summed together.


Singular matrices
^^^^^^^^^^^^^^^^^

Very occasionally, a singular matrix will pop out of these equations. Then what
I do is calculate the slope vectors at the endpoints and then try many
reasonable lengths for those vectors and see which yields the best approximation
to the original curve (this gives us our new control points).

This is very, very slow.

--------------------------------------------------------------------------------


.. [#f1] Guessing values for t

   FontForge approximates the lengths of the two splines being merged. If
   :math:`\mathrm{Point}_i = \mathrm{Spline1}(old-t_i)`, then we approximate
   :math:`t_i` by

   .. math::

      t_i = old - t_i \frac{\mathrm{len}(\mathrm{spline1})}
                           {\mathrm{len}(\mathrm{spline1}) + \mathrm{len}(\mathrm{spline2})}

   and if :math:`\mathrm{Point}_i = \mathrm{Spline2(old-t_i)}`

   .. math::

      t_i = \frac{\mathrm{len}(\mathrm{spline1})}
                 {\mathrm{len}(\mathrm{spline1}) + \mathrm{len}(\mathrm{spline2})} +
            old-t_i
            \frac{\mathrm{len}(\mathrm{spline2})}
                 {\mathrm{len}(\mathrm{spline1}) + \mathrm{len}(\mathrm{spline2})}

   That is we do a linear interpolation of t based on the relative lengths of the
   two splines.

--------------------------------------------------------------------------------


.. _pfaeditmath.Stroke:

Calculating the outline of a stroked path
-----------------------------------------

.. note::
   While some of this section is still accurate, most of it describes an
   earlier version of the Expand Stroke facility. See :doc:`here <stroke>`
   for an up-to-date description of the current algorithm.


A circular pen
^^^^^^^^^^^^^^

PostScript supports several variants on the theme of a circular pen, and
FontForge tries to emulate them all. Basically PostScript "stroke"s a path at a
certain width by:

| at every location on the curve
|     find the normal vector at that location
|     find the two points which are width/2 away from the curve
|     filling in between those two points
| end

This is essentially what a circular pen does. The only aberrations appear at the
end-points of a contour, or at points where two splines join but their slopes
are not continuous. PostScript allows the user to specify the behavior at joints
and at end-points.

|expand-pre| --> |expand-post|

FontForge can't deal with an infinite number of locations, so it samples the
curve (approximately every em unit), and finds the two normal points. These are
on the edge of the area to be stroked, so FontForge can approximate new contours
from these edge points (using the
:ref:`above algorithm <pfaeditmath.Approximating>`).

PostScript pens can end in

* A butt edge -- this is easy, we just draw a line from the end of one spline to
  the end of the other
* A rounded edge -- here we just draw a semi-circle (making sure it goes in the
  right direction).
* A square edge -- just draw lines continuing the two splines, moving with the
  same slope and width/2 units long, and then join those end-points with a
  straight line.

Things are a bit more complex at a joint |expand-joint-pre| --> |expand-joint-post|
, the green lines in the right image show where the path would have gone had it
not been constrained by a joint, so on the inside of the joint FontForge must
figure out where this intersection occurs. While on the outside FontForge must
figure out either a round, miter or bevelled edge.

Unfortunately, the normal points are not always on the edge of the filled
region. If the curve makes a sharp bend, then one of the normal points may end
up inside the pen when it is centered somewhere else on the original contour
(similar to the case of a joint above).

So FontForge makes another pass through the edge points and figures out which
ones are actually internal points. After that it will approximate contours.

Now if we start with an open contour, (a straight line, for example) then we
will end up with one closed contour. While if we start with a closed contour we
will end up with two closed contours (one on the inside, and one on the
outside). Unless there are places where the curve doubles back on itself, then
when can get arbitrarily many closed contours.

.. |expand-pre| image:: /images/expand-pre.png
.. |expand-post| image:: /images/expand-post.png
.. |expand-joint-pre| image:: /images/expand-joint-pre.png
.. |expand-joint-post| image:: /images/expand-joint-post.png

 
An elliptical pen
^^^^^^^^^^^^^^^^^

This is really just the same as a circular pen. Let us say we want an ellipse
which is twice as wide as it is high. Then before stroking the path, let's scale
it to 50% in the horizontal direction, then stroke it with a circular pen, and
then scale it back by 200% horizontally. The result will be as if we had used an
elliptical pen.

Obviously if the ellipse is at an angle to the glyph's axes, we must apply a
more complicated transformation which involves both rotation and scaling.


A rectangular pen (a calligraphic pen)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Things are subtly different between a rectangular pen and a circular pen. We can
no longer just find the points which are a given distance away and normal to the
curve. Except where the spline is parallel to one edge of the pen, a the outer
contour of a rectangular pen will be stroked by one of its end-points. So all we
need do is figure out where a spline is parallel to the pen's sides, and look at
the problem in little chunks between those difficult points.

If we are between difficult points then life is very simple indeed. The edge
will always be stroked by the same end-point, which is a fixed distance from the
center of the pen, so all we need to do is translate the original spline by this
distance (and then fix it up so that t goes from [0,1], but that's another easy
transformation).

When we reach a point where the spline's slope is parallel to one edge of the
pen, then on the outside path we draw a copy of that edge of of the pen, and on
the inside edge we calculate a join as above.


An arbitrary convex polygonal pen
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The same method which works for a rectangle can be extended without too much
difficulty to any convex polygon. (MetaFont fonts can be drawn with such a pen.
I don't know if any are)


A pen of variable width
^^^^^^^^^^^^^^^^^^^^^^^

FontForge does not currently support this (some of the assumptions behind this
algorithm are broken if the pen changes shape too rapidly).


A pen at a varying angle
^^^^^^^^^^^^^^^^^^^^^^^^

FontForge does not support this.