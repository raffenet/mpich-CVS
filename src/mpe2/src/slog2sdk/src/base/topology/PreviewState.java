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
import java.util.Arrays;

import base.drawable.CoordPixelXform;
import base.drawable.CategoryWeight;
import base.drawable.DrawnBox;
import base.drawable.Category;

public class PreviewState
{
    private static StateBorder BorderStyle  = StateBorder.WHITE_RAISED_BORDER;

    public static void setBorderStyle( final StateBorder state_border )
    {
        BorderStyle = state_border;
    }

    public  static final  String DECRE_WEIGHT_ORDER    = "DECRE_WEIGHT_ORDER";
    private static final  int    DECRE_WEIGHT_ORDER_ID = 0;
    public  static final  String DECRE_LEGEND_ORDER    = "DECRE_LEGEND_ORDER";
    private static final  int    DECRE_LEGEND_ORDER_ID = 1;
    public  static final  String MOST_LEGENDS_ORDER    = "MOST_LEGENDS_ORDER";
    private static final  int    MOST_LEGENDS_ORDER_ID = 2;

    private static        int    DisplayType           = DECRE_WEIGHT_ORDER_ID;

    public static void setDisplayType( String new_display_type )
    {
        if ( new_display_type.equals( MOST_LEGENDS_ORDER ) )
            DisplayType = MOST_LEGENDS_ORDER_ID;
        else if ( new_display_type.equals( DECRE_LEGEND_ORDER ) )
            DisplayType = DECRE_LEGEND_ORDER_ID;
        else if ( new_display_type.equals( DECRE_WEIGHT_ORDER ) )
            DisplayType = DECRE_WEIGHT_ORDER_ID;
        else
            DisplayType = DECRE_WEIGHT_ORDER_ID;
    }

    private static        int    MinCategoryHeight   = 2;  

    public static void setMinCategoryHeight( int new_min_category_height )
    {
        MinCategoryHeight  = new_min_category_height;
    }

    /*
        Draw a Rectangle between left-upper vertex (start_time, start_ypos) 
        and right-lower vertex (final_time, final_ypos)
        Assume caller guarantees the order of timestamps and ypos, such that
        start_time <= final_time  and  start_ypos <= final_ypos.
    */
    private static int  drawForward( Graphics2D g,
                                     CategoryWeight[] twgts, Insets insets,
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
            
        int iWidth  = iTail-iHead+1;
        int jHeight = jTail-jHead+1;

        CategoryWeight  twgt = null;
        int             idx, twgts_length;
        float           height_per_wgt;
        int             jLevel, jDelta;

        twgts_length = twgts.length;
        if ( DisplayType == MOST_LEGENDS_ORDER_ID ) {
            int num_visible_twgts = 0;
            for ( idx = 0; idx < twgts_length; idx++ ) {
                if ( twgts[ idx ].getCategory().isVisible() )
                    num_visible_twgts++;
            }
            jDelta = (int) ( (float) jHeight / num_visible_twgts );
            if ( jDelta < MinCategoryHeight )
                jDelta = MinCategoryHeight;
            // set sub-rectangles' height from the bottom, ie. jHead+jTail
            jLevel = jHead + jHeight;  // jLevel = jTail + 1
            for ( idx = twgts_length-1; idx >= 0; idx-- ) {
                twgt = twgts[ idx ];
                if ( twgt.getCategory().isVisible() ) {
                    if ( jLevel > jHead ) {
                        if ( jLevel-jDelta >= jHead ) {
                            jLevel  -= jDelta;
                            twgt.setPixelHeight( jDelta );
                        }
                        else {
                            twgt.setPixelHeight( jLevel - jHead );
                            jLevel = jHead;
                        }
                    }
                    else
                        twgt.setPixelHeight( 0 );
                }
                else
                    twgt.setPixelHeight( 0 );
            }
        }
        else {
            // Compute the pixel height per unit weight
            float  tot_wgt = 0.0f;
            for ( idx = 0; idx < twgts_length; idx++ ) {
                twgt = twgts[ idx ];
                if ( twgt.getCategory().isVisible() )
                    tot_wgt += twgt.getWeight();
            }
            height_per_wgt = (float) jHeight / tot_wgt;

            // Assume the input twgt[] is arranged in WEIGHT_ORDER
            // Arrays.sort( twgts, CategoryWeight.WEIGHT_ORDER );

            // set sub-rectangles' height from the bottom, ie. jHead+jTail
            jLevel = jHead + jHeight;  // jLevel = jTail + 1
            for ( idx = twgts_length-1; idx >= 0; idx-- ) {
                twgt = twgts[ idx ];
                if ( twgt.getCategory().isVisible() ) {
                    jDelta = (int) (height_per_wgt * twgt.getWeight() + 0.5f);
                    if ( jDelta > 0 ) {
                        if ( jLevel > jHead ) {
                            if ( jLevel-jDelta >= jHead ) {
                                jLevel  -= jDelta;
                                twgt.setPixelHeight( jDelta );
                            }
                            else {
                                twgt.setPixelHeight( jLevel - jHead );
                                jLevel = jHead;
                            }
                        }
                        else
                            twgt.setPixelHeight( 0 );
                    }
                    else
                        twgt.setPixelHeight( 0 );
                }
                else
                    twgt.setPixelHeight( 0 );
            }
        }

        if ( DisplayType == DECRE_LEGEND_ORDER_ID )
            Arrays.sort( twgts, CategoryWeight.INDEX_ORDER );
        else // if ( DisplayType == DECRE_WEIGHT_ORDER || MOST_LEGENDS_ORDER )
            Arrays.sort( twgts, CategoryWeight.WEIGHT_ORDER );

        // Fill the color of the sub-rectangles from the bottom, ie. jHead+jTail
        int num_sub_rects_drawn = 0;
        jLevel = jHead + jHeight;  // jLevel = jTail + 1
        for ( idx = twgts_length-1; idx >= 0; idx-- ) {
            twgt     = twgts[ idx ];
            jDelta   = twgt.getPixelHeight();
            if ( jDelta > 0 ) {
                jLevel  -= jDelta;
                g.setColor( twgt.getCategory().getColor() );
                g.fillRect( iHead, jLevel, iWidth, jDelta );
                num_sub_rects_drawn++;
            }
        }

        if ( num_sub_rects_drawn > 0 )
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
    private static Category isPixelIn( CategoryWeight[] twgts, Insets insets,
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

        int jHead, jTail, jHeight;
        jHead    = jStart;
        jTail    = jFinal;
        jHeight  = jTail-jHead+1;

        CategoryWeight  twgt = null;
        int             idx, twgts_length;
        twgts_length = twgts.length;

        // Locate the sub-rectangle from the bottom, ie. jHead+jTail
        int jLevel, jDelta;
        jLevel = jHead + jHeight;  // jLevel = jTail + 1
        for ( idx = twgts_length-1; idx >= 0; idx-- ) {
            twgt     = twgts[ idx ];
            jDelta   = twgt.getPixelHeight(); 
            if ( jDelta > 0 ) {
                jLevel  -= jDelta;
                if ( pt_y >= jLevel )
                    return twgt.getCategory();
            }
        }

        return null; // mean failure, need something other than null
    }


    public static int  draw( Graphics2D g,
                             CategoryWeight[] twgts, Insets insets,
                             CoordPixelXform    coord_xform,
                             DrawnBox           last_drawn_pos,
                             double start_time, float start_ypos,
                             double final_time, float final_ypos )
    {
         if ( start_time < final_time ) {
             if ( start_ypos < final_ypos )
                 return drawForward( g, twgts, insets,
                                     coord_xform, last_drawn_pos,
                                     start_time, start_ypos,
                                     final_time, final_ypos );
             else
                 return drawForward( g, twgts, insets,
                                     coord_xform, last_drawn_pos,
                                     start_time, final_ypos,
                                     final_time, start_ypos );
         }
         else {
             if ( start_ypos < final_ypos )
                 return drawForward( g, twgts, insets,
                                     coord_xform, last_drawn_pos,
                                     final_time, start_ypos,
                                     start_time, final_ypos );
             else
                 return drawForward( g, twgts, insets,
                                     coord_xform, last_drawn_pos,
                                     final_time, final_ypos,
                                     start_time, start_ypos );
         }
    }

    public static Category containsPixel( CategoryWeight[] twgts, Insets insets,
                                          CoordPixelXform coord_xform, Point pt,
                                          double start_time, float start_ypos,
                                          double final_time, float final_ypos )
    {
         if ( start_time < final_time ) {
             if ( start_ypos < final_ypos )
                 return isPixelIn( twgts, insets, coord_xform, pt,
                                   start_time, start_ypos,
                                   final_time, final_ypos );
             else
                 return isPixelIn( twgts, insets, coord_xform, pt,
                                   start_time, final_ypos,
                                   final_time, start_ypos );
         }
         else {
             if ( start_ypos < final_ypos )
                 return isPixelIn( twgts, insets, coord_xform, pt,
                                   final_time, start_ypos,
                                   start_time, final_ypos );
             else
                 return isPixelIn( twgts, insets, coord_xform, pt,
                                   final_time, final_ypos,
                                   start_time, start_ypos );
         }
    }
}
