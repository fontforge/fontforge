psMat
=====

.. default-domain:: py

.. module:: psMat
   :synopsis: Provides quick access to some useful transformations expressed as PostScript matrices

.. function:: identity()

   returns an identity matrix as a 6 element tuple

.. function:: compose(mat1, mat2)

   returns a matrix which is the composition of the two input transformations

.. function:: inverse(mat)

   returns a matrix which is the inverse of the input transformation.
   (Note: There will not always be an inverse)

.. function:: rotate(theta)

   returns a matrix which will rotate by ``theta``. ``theta`` is expressed
   in radians

.. function:: scale(x[, y])

   returns a matrix which will scale by ``x`` in the horizontal direction and
   ``y`` in the vertical. If ``y`` is omitted, it will scale by the same
   amount (``x``) in both directions

.. function:: skew(theta)
   
   returns a matrix which will skew by ``theta`` (to produce an oblique font).
   ``theta`` is expressed in radians

.. function::  translate(x, y)

   returns a matrix which will translate by ``x`` in the horizontal direction
   and ``y`` in the vertical
