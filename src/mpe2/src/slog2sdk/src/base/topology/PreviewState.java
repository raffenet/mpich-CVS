/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.topology;

import java.awt.Graphics2D;
import java.awt.Insets;
import java.awt.Color;
import java.awt.Point;
import java.util.List;
import java.util.ListIterator;

import base.drawable.CoordPixelXform;
import base.drawable.CategoryWeight;
import base.drawable.DrawnBox;
import base.drawable.Category;

public class PreviewState extends State
{
    /*
        Draw a Rectangle between left-upper vertex (start_time, start_ypos) 
        and right-lower vertex (final_time, final_ypos)
        Assume caller guarantees the order of timestamps and ypos, such that
        start_time <= final_time  and  start_ypos <= final_ypos.
    */
    private static int  drawForward( Graphics2D g,
                                     List twgt_list, Insets insets,
                                     CoordPixelXform    coord_xform,
                                     DrawnBox           last_drawn_pos,
                                     double start_time, float start_ypos,
                                     double final_time, float final_ypos )
    {
        int      iStart, jStart, iFinal, jFinal;
        iStart   = coord_xform.convertTimeToPixel( start_time );
        iFinal   = coord_xform.convertTimeToPixel( final_time );

        /* Determine if State should be drawn */
        if ( last_drawn_pos.coversState( iStart, iFinal ) )
            return 0; // too small to be drawn in previously drawn location
        last_drawn_pos.set( iStart, iFinal );

        jStart   = coord_xform.convertRowToPixel( start_ypos );
        jFinal   = coord_xform.convertRowToPixel( final_ypos );

        if ( insets != null ) {
            iStart += insets.left;
            iFinal -= insets.right;
            jStart += insets.top;
            jFinal -= insets.bottom;
        }

        boolean  isStartVtxInImg, isFinalVtxInImg;
        isStartVtxInImg = ( iStart >= 0 ) ;
        isFinalVtxInImg = ( iFinal <  coord_xform.getImageWidth() );

        int iHead, iTail, jHead, jTail;
        // jHead = slope * ( iHead - iStart ) + jStart
        if ( isStartVtxInImg )
            iHead = iStart;
        else
            iHead = 0;
            // iHead = -1;
        jHead    = jStart;

        // jTail = slope * ( iTail - iFinal ) + jFinal
        if ( isFinalVtxInImg )
            iTail = iFinal;
        else
            iTail = coord_xform.getImageWidth() - 1;
            // iTail = coord_xform.getImageWidth();
        jTail    = jFinal;
            
        int iwidth  = iTail-iHead+1;
        int jheight = jTail-jHead+1;

        // Compute the pixel height per unit weight
        CategoryWeight  twgt = null;
        float           tot_wgt = 0.0f;
        ListIterator twgts_itr = twgt_list.listIterator();
        while ( twgts_itr.hasNext() ) {
            twgt  = (CategoryWeight) twgts_itr.next();
            if ( twgt.getCategory().isVisible() )
                tot_wgt += twgt.getWeight();
        }
        float  height_per_wgt = (float) jheight / tot_wgt;

        // Fill the color of the sub-rectangles
        jHead += jheight;
        while ( twgts_itr.hasPrevious() ) {
            twgt  = (CategoryWeight) twgts_itr.previous();
            if ( twgt.getCategory().isVisible() ) {
                jheight  = (int) (height_per_wgt * twgt.getWeight());
                if ( jheight > 0 ) {
                    jHead -= jheight;
                    g.setColor( twgt.getCategory().getColor() );
                    g.fillRect( iHead, jHead, iwidth, jheight );
                }
            }
        }

        BorderStyle.paintStateBorder( g, iHead, jHead, isStartVtxInImg,
                                         iTail, jTail, isFinalVtxInImg );
        return 1;
    }

    /*
        Check if a point in pixel coordinate is in a Rectangle
        specified between left-upper vertex (start_time, start_ypos) 
        and right-lower vertex (final_time, final_ypos)
        Assume caller guarantees the order of timestamps and ypos, such that
        start_time <= final_time  and  start_ypos <= final_ypos
    */
    private static Category isPixelIn( List twgt_list, Insets insets,
                                       CoordPixelXform coord_xform, Point pt,
                                       double start_time, float start_ypos,
                                       double final_time, float final_ypos )
    {
        int      iStart, jStart, iFinal, jFinal;
        int      pt_x, pt_y;

        pt_y     = pt.y;

        jStart   = coord_xform.convertRowToPixel( start_ypos );
        if ( pt_y < jStart  )
            return null;

        jFinal   = coord_xform.convertRowToPixel( final_ypos );
        if ( pt_y > jFinal )
            return null;

        pt_x     = pt.x;

        iStart   = coord_xform.convertTimeToPixel( start_time );
        if ( pt_x < iStart )
            return null;

        iFinal   = coord_xform.convertTimeToPixel( final_time );
        if ( pt_x > iFinal )
            return null;

        //  If the code gets to here, it means the pixel is within the Shadow.
        if ( insets != null ) {
            iStart += insets.left;
            iFinal -= insets.right;
            jStart += insets.top;
            jFinal -= insets.bottom;
        }

        int jHead, jTail, jheight;
        jHead    = jStart;
        jTail    = jFinal;
        jheight  = jTail-jHead+1;

        // Compute the pixel height per unit weight
        CategoryWeight  twgt = null;
        float           tot_wgt = 0.0f;
        ListIterator twgts_itr = twgt_list.listIterator();
        while ( twgts_itr.hasNext() ) {
            twgt  = (CategoryWeight) twgts_itr.next();
            if ( twgt.getCategory().isVisible() )
                tot_wgt += twgt.getWeight();
        }
        float  height_per_wgt = (float) jheight / tot_wgt;

        // Fill the color of the sub-rectangles
        jHead += jheight;
        while ( twgts_itr.hasPrevious() ) {
            twgt  = (CategoryWeight) twgts_itr.previous();
            if ( twgt.getCategory().isVisible() ) {
                jheight  = (int) (height_per_wgt * twgt.getWeight());
                if ( jheight > 0 ) {
                    jHead -= jheight;
                    if ( pt_y >= jHead )
                        return twgt.getCategory();
                }
            }
        }

        return null; // mean failure, need something other than null
    }


    public static int  draw( Graphics2D g, List twgt_list, Insets insets,
                             CoordPixelXform    coord_xform,
                             DrawnBox           last_drawn_pos,
                             double start_time, float start_ypos,
                             double final_time, float final_ypos )
    {
         if ( start_time < final_time ) {
             if ( start_ypos < final_ypos )
                 return drawForward( g, twgt_list, insets,
                                     coord_xform, last_drawn_pos,
                                     start_time, start_ypos,
                                     final_time, final_ypos );
             else
                 return drawForward( g, twgt_list, insets,
                                     coord_xform, last_drawn_pos,
                                     start_time, final_ypos,
                                     final_time, start_ypos );
         }
         else {
             if ( start_ypos < final_ypos )
                 return drawForward( g, twgt_list, insets,
                                     coord_xform, last_drawn_pos,
                                     final_time, start_ypos,
                                     start_time, final_ypos );
             else
                 return drawForward( g, twgt_list, insets,
                                     coord_xform, last_drawn_pos,
                                     final_time, final_ypos,
                                     start_time, start_ypos );
         }
    }

    public static Category containsPixel( List twgt_list, Insets insets,
                                          CoordPixelXform coord_xform, Point pt,
                                          double start_time, float start_ypos,
                                          double final_time, float final_ypos )
    {
         if ( start_time < final_time ) {
             if ( start_ypos < final_ypos )
                 return isPixelIn( twgt_list, insets, coord_xform, pt,
                                   start_time, start_ypos,
                                   final_time, final_ypos );
             else
                 return isPixelIn( twgt_list, insets, coord_xform, pt,
                                   start_time, final_ypos,
                                   final_time, start_ypos );
         }
         else {
             if ( start_ypos < final_ypos )
                 return isPixelIn( twgt_list, insets, coord_xform, pt,
                                   final_time, start_ypos,
                                   start_time, final_ypos );
             else
                 return isPixelIn( twgt_list, insets, coord_xform, pt,
                                   final_time, final_ypos,
                                   start_time, start_ypos );
         }
    }
}
