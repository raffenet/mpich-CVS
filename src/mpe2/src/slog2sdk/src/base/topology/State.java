/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.topology;

import java.awt.Graphics2D;
import java.awt.Stroke;
import java.awt.Color;
import java.awt.Point;
import base.drawable.CoordPixelXform;

public class State
{
    /*
        Draw a Rectangle between left-upper vertex (start_time, start_ypos) 
        and right-lower vertex (final_time, final_ypos)
        Assume caller guarantees the order of timestamps and ypos, such that
        start_time <= final_time  and  start_ypos <= final_ypos.
    */
    private static void drawForward( Graphics2D g, Color color, Stroke stroke,
                                     CoordPixelXform coord_xform,
                                     double start_time, float start_ypos,
                                     double final_time, float final_ypos )
    {
        int      iStart, jStart, iFinal, jFinal;
        iStart   = coord_xform.convertTimeToPixel( start_time );
        jStart   = coord_xform.convertRowToPixel( start_ypos );

        iFinal   = coord_xform.convertTimeToPixel( final_time );
        jFinal   = coord_xform.convertRowToPixel( final_ypos );

        boolean  isStartVtxInImg, isFinalVtxInImg;
        isStartVtxInImg = iStart >= 0 ;
        isFinalVtxInImg = iFinal <  coord_xform.getImageWidth();

        int iHead, iTail, jHead, jTail;
        // jHead = slope * ( iHead - iStart ) + jStart
        if ( isStartVtxInImg )
            iHead = iStart;
        else
            iHead = 0;
        jHead = jStart;

        // jTail = slope * ( iTail - iFinal ) + jFinal
        if ( isFinalVtxInImg )
            iTail = iFinal;
        else
            iTail = coord_xform.getImageWidth() - 1;
        jTail = jFinal;

        // Fill the color of the rectangle
        if ( iTail > iHead && jTail > jHead ) {
            g.setColor( color );
            g.fillRect( iHead, jHead, iTail-iHead, jTail-jHead );
        }

        Stroke orig_stroke = null;
        if ( stroke != null ) {
            orig_stroke = g.getStroke();
            g.setStroke( stroke );
        }

        // Draw the outline of the rectangle
        g.setColor( Color.white );
        g.drawLine( iHead, jHead, iTail, jHead );
        g.drawLine( iHead, jTail, iTail, jTail );

        if ( isStartVtxInImg )
            g.drawLine( iHead, jHead, iHead, jTail );
        if ( isFinalVtxInImg )
            g.drawLine( iTail, jHead, iTail, jTail );

        if ( stroke != null )
            g.setStroke( orig_stroke );
    }

    /*
        Check if a point in pixel coordinate is in a Rectangle
        specified between left-upper vertex (start_time, start_ypos) 
        and right-lower vertex (final_time, final_ypos)
        Assume caller guarantees the order of timestamps and ypos, such that
        start_time <= final_time  and  start_ypos <= final_ypos
    */
    private static boolean isPixelIn( CoordPixelXform coord_xform, Point pt,
                                      double start_time, float start_ypos,
                                      double final_time, float final_ypos )
    {
        int      iStart, jStart, iFinal, jFinal;
        int      pt_x, pt_y;

        pt_y     = pt.y;

        jStart   = coord_xform.convertRowToPixel( start_ypos );
        if ( pt_y < jStart  )
            return false;

        jFinal   = coord_xform.convertRowToPixel( final_ypos );
        if ( pt_y > jFinal )
            return false;

        pt_x     = pt.x;

        iStart   = coord_xform.convertTimeToPixel( start_time );
        if ( pt_x < iStart )
            return false;

        iFinal   = coord_xform.convertTimeToPixel( final_time );
        if ( pt_x > iFinal )
            return false;

        return true;
    }


    public static void draw( Graphics2D g, Color color, Stroke stroke,
                             CoordPixelXform coord_xform,
                             double start_time, float start_ypos,
                             double final_time, float final_ypos )
    {
         if ( start_time < final_time ) {
             if ( start_ypos < final_ypos )
                 drawForward( g, color, stroke, coord_xform,
                              start_time, start_ypos,
                              final_time, final_ypos );
             else
                 drawForward( g, color, stroke, coord_xform,
                              start_time, final_ypos,
                              final_time, start_ypos );
         }
         else {
             if ( start_ypos < final_ypos )
                 drawForward( g, color, stroke, coord_xform,
                              final_time, start_ypos,
                              start_time, final_ypos );
             else
                 drawForward( g, color, stroke, coord_xform,
                              final_time, final_ypos,
                              start_time, start_ypos );
         }
    }

    public static boolean containsPixel( CoordPixelXform coord_xform, Point pt,
                                         double start_time, float start_ypos,
                                         double final_time, float final_ypos )
    {
         if ( start_time < final_time ) {
             if ( start_ypos < final_ypos )
                 return isPixelIn( coord_xform, pt,
                                   start_time, start_ypos,
                                   final_time, final_ypos );
             else
                 return isPixelIn( coord_xform, pt,
                                   start_time, final_ypos,
                                   final_time, start_ypos );
         }
         else {
             if ( start_ypos < final_ypos )
                 return isPixelIn( coord_xform, pt,
                                   final_time, start_ypos,
                                   start_time, final_ypos );
             else
                 return isPixelIn( coord_xform, pt,
                                   final_time, final_ypos,
                                   start_time, start_ypos );
         }
    }
}
