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
import java.awt.geom.Line2D;
import base.drawable.CoordPixelXform;
import base.drawable.DrawnBox;

public class Event
{
    private static       int     Base_Half_Width        = 3;
    private static       double  Max_LineSeg2Pt_DistSQ  = 10.0d;

    //  For Viewer
    public static void setBaseWidth( int new_width )
    {
        Base_Half_Width = new_width / 2;
        if ( Base_Half_Width < 1 )
            Base_Half_Width = 1;
    }

    public static void setPixelClosenessTolerance( int pix_dist )
    {
        // add 1 at the end so containsPixel() can use "<" instead of "<="
        Max_LineSeg2Pt_DistSQ = (double) ( pix_dist * pix_dist + 1 );
    }

    public static int  draw( Graphics2D g, Color color, Stroke stroke,
                             CoordPixelXform    coord_xform,
                             DrawnBox           last_drawn_pos,
                             double point_time, float peak_ypos,
                             float start_ypos, float final_ypos )
    {
        int      iPoint, jPeak, jStart, jFinal;
        iPoint   = coord_xform.convertTimeToPixel( point_time );

        /* Determine if Event should be drawn */
        if ( last_drawn_pos.coversEvent( iPoint ) )
            return 0; // Event has been drawn at the same location before
        last_drawn_pos.set( iPoint );

        boolean  isPointVtxInImg;
        isPointVtxInImg = iPoint > 0 && iPoint < coord_xform.getImageWidth();

        if ( ! isPointVtxInImg )
            return 0;

        jPeak    = coord_xform.convertRowToPixel( peak_ypos );
        jStart   = coord_xform.convertRowToPixel( start_ypos );
        jFinal   = coord_xform.convertRowToPixel( final_ypos );

        Stroke orig_stroke = null;
        if ( stroke != null ) {
            orig_stroke = g.getStroke();
            g.setStroke( stroke );
        }

        g.setColor( color );

        int  iLeft, iRight;
        g.drawLine( iPoint, jStart, iPoint, jPeak );
        iLeft  = iPoint - Base_Half_Width;
        iRight = iPoint + Base_Half_Width;
        g.drawLine( iLeft, jFinal, iRight, jFinal );
        if ( jPeak != jFinal ) {
            g.drawLine( iPoint, jPeak, iLeft, jFinal );
            g.drawLine( iPoint, jPeak, iRight, jFinal );
        }

        if ( stroke != null )
            g.setStroke( orig_stroke );

        return 1;
    }

/*
    public static boolean  containsPixel( CoordPixelXform coord_xform, Point pt,
                                          double point_time,
                                          float peak_ypos, float final_ypos )
    {
        int      iPoint, iLeft, iRight, jPeak, jFinal;
        int      pt_x, pt_y;

        pt_y     = pt.y;

        jPeak    = coord_xform.convertRowToPixel( peak_ypos );
        if ( pt_y < jPeak  )
            return false;

        jFinal   = coord_xform.convertRowToPixel( final_ypos );
        if ( pt_y > jFinal )
            return false;

        pt_x     = pt.x;

        iPoint   = coord_xform.convertTimeToPixel( point_time );
        iLeft    = iPoint - Base_Half_Width;
        if ( pt_x < iLeft )
            return false;

        iRight   = iPoint + Base_Half_Width;
        if ( pt_x > iRight )
            return false;

        return true;
    }
*/
    public static boolean  containsPixel( CoordPixelXform coord_xform, Point pt,
                                          double point_time,
                                          float start_ypos, float final_ypos )
    {
        double   xPoint, yStart, yFinal;
        double   xPt, yPt;
        double   distSQ;

        xPoint   = (double) coord_xform.convertTimeToPixel( point_time );
        yStart   = (double) coord_xform.convertRowToPixel( start_ypos );
        yFinal   = (double) coord_xform.convertRowToPixel( final_ypos );

        xPt      = (double) pt.x;
        yPt      = (double) pt.y;

        distSQ   = Line2D.ptSegDistSq( xPoint, yStart, xPoint, yFinal,
                                       xPt, yPt );
        return distSQ < Max_LineSeg2Pt_DistSQ;
    }
}
