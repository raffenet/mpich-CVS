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
import base.statistics.CategoryTimeBox;

public class SummaryState
{
    // private static StateBorder BorderStyle = StateBorder.COLOR_XOR_BORDER;
    private static StateBorder BorderStyle = StateBorder.WHITE_RAISED_BORDER;

    public static void setBorderStyle( final StateBorder state_border )
    {
        BorderStyle = state_border;
    }

    public  static final String FIT_MOST_LEGENDS
                                = "FitMostLegends";
    private static final int    FIT_MOST_LEGENDS_ID     = 0;
    public  static final String OVERLAP_INCLUSION
                                = "OverlapInclusionRatio";
    private static final int    OVERLAP_INCLUSION_ID    = 1;
    public  static final String CUMULATIVE_INCLUSION
                                = "CumulativeInclusionRatio";
    private static final int    CUMULATIVE_INCLUSION_ID = 2;
    public  static final String OVERLAP_EXCLUSION
                                = "OverlapExclusionRatio";
    private static final int    OVERLAP_EXCLUSION_ID    = 3;
    public  static final String CUMULATIVE_EXCLUSION
                                = "CumulativeExclusionRatio";
    private static final int    CUMULATIVE_EXCLUSION_ID = 4;

    private static       int    DisplayType             = OVERLAP_INCLUSION_ID;

    public static void setDisplayType( String new_display_type )
    {
        if ( new_display_type.equals( FIT_MOST_LEGENDS ) )
            DisplayType = FIT_MOST_LEGENDS_ID;
        else if ( new_display_type.equals( OVERLAP_INCLUSION ) )
            DisplayType = OVERLAP_INCLUSION_ID;
        else if ( new_display_type.equals( CUMULATIVE_INCLUSION ) )
            DisplayType = CUMULATIVE_INCLUSION_ID;
        else if ( new_display_type.equals( OVERLAP_EXCLUSION ) )
            DisplayType = OVERLAP_EXCLUSION_ID;
        else if ( new_display_type.equals( CUMULATIVE_EXCLUSION ) )
            DisplayType = CUMULATIVE_EXCLUSION_ID;
        else
            DisplayType = OVERLAP_INCLUSION_ID;
    }

    public static boolean isDisplayTypeEqualWeighted()
    {
        return DisplayType == FIT_MOST_LEGENDS_ID;
    }

    public static boolean isDisplayTypeExclusiveRatio()
    {
        return    DisplayType == OVERLAP_EXCLUSION_ID
               || DisplayType == CUMULATIVE_EXCLUSION_ID;
    }

    public static boolean isDisplayTypeCumulative()
    {
        return    DisplayType == CUMULATIVE_INCLUSION_ID
               || DisplayType == CUMULATIVE_EXCLUSION_ID;
    }

    private static        int    MinCategoryHeight          = 2;  
    private static        int    MinCategorySeparation      = 4;  

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
                                     Color color, Insets insets,
                                     CoordPixelXform coord_xform,
                                     double start_time, float start_ypos,
                                     double final_time, float final_ypos )
    {
        int      iStart, jStart, iFinal, jFinal;
        iStart   = coord_xform.convertTimeToPixel( start_time );
        iFinal   = coord_xform.convertTimeToPixel( final_time );

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

        // Fill the color of the rectangle
        g.setColor( color );
        g.fillRect( iHead, jHead, iTail-iHead+1, jTail-jHead+1 );

        BorderStyle.paintStateBorder( g, color,
                                      iHead, jHead, isStartVtxInImg,
                                      iTail, jTail, isFinalVtxInImg );
        return 1;
    }

    /*
        Check if a point in pixel coordinate is in a Rectangle
        specified between left-upper vertex (start_time, start_ypos) 
        and right-lower vertex (final_time, final_ypos)
        Assume caller guarantees the order of timestamps and ypos, such that
        start_time <= final_time  and  start_ypos <= final_ypos

        Same as State.isPixelIn()
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



    public static void setTimeBoundingBox( CategoryTimeBox[]  typeboxes,
                                           double             starttime,
                                           double             finaltime )
    {
        CategoryTimeBox typebox;
        boolean         isInclusive;
        double          prev_time, interval, duration;
        int             idx;

        if ( isDisplayTypeExclusiveRatio() )
            Arrays.sort( typeboxes, CategoryTimeBox.EXCL_RATIO_ORDER );
        else // OverlapInclusionRatio, CumulativeInclusionRatio, FitMostLegends
            Arrays.sort( typeboxes, CategoryTimeBox.INCL_RATIO_ORDER );

        /*
           CategoryTimeBox[] is in ascending order of the respective ratio
           set TimeBoundingBox of CategoryTimeBox[] in descending ratio order
        */

        if ( isDisplayTypeEqualWeighted() ) {
            prev_time  = starttime;
            interval   = ( finaltime - starttime ) / typeboxes.length;
            for ( idx = typeboxes.length-1; idx >= 0; idx-- ) {
                typebox   = typeboxes[ idx ];
                typebox.setEarliestTime( prev_time );
                typebox.setLatestFromEarliest( interval );
                prev_time = typebox.getLatestTime();
            }
        }
        else {
            isInclusive  = ! isDisplayTypeExclusiveRatio();
            if ( isDisplayTypeCumulative() ) { // CumulativeXXclusionRatio
                prev_time  = starttime;
                duration   = finaltime - starttime;
                for ( idx = typeboxes.length-1; idx >= 0; idx-- ) {
                    typebox   = typeboxes[ idx ];
                    interval  = duration
                              * typebox.getCategoryRatio( isInclusive ) ;
                    typebox.setEarliestTime( prev_time );
                    typebox.setLatestFromEarliest( interval );
                    prev_time = typebox.getLatestTime();
               }
            }
            else {  // OverlapInclusionRatio, OverlapExclusiveRatio
                duration   = finaltime - starttime;
                for ( idx = typeboxes.length-1; idx >= 0; idx-- ) {
                    typebox   = typeboxes[ idx ];
                    interval  = duration
                              * typebox.getCategoryRatio( isInclusive ) ;
                    typebox.setEarliestTime( starttime );
                    typebox.setLatestFromEarliest( interval );
               }
            }
        }
    }

    public  static int  draw( Graphics2D  g, Insets  insets,
                              CategoryTimeBox[]  typeboxes,
                              CoordPixelXform  coord_xform,
                              float start_ypos, float final_ypos )
    {
        CategoryTimeBox  typebox;
        Color            color;
        double           head_time, tail_time;
        float            head_ypos, tail_ypos, gap_ypos;
        int              count, idx;

        if ( start_ypos < final_ypos ) {
            head_ypos  = start_ypos;
            tail_ypos  = final_ypos;
        }
        else {
            head_ypos  = final_ypos;
            tail_ypos  = start_ypos;
        }

        // Draw CategoryTimeBox[] in descending ratio order
        count = 0;
        if (    isDisplayTypeEqualWeighted()
             || isDisplayTypeCumulative() ) {
            for ( idx = typeboxes.length-1; idx >= 0; idx-- ) {
                typebox    = typeboxes[ idx ];
                color      = typebox.getCategoryColor();
                head_time  = typebox.getEarliestTime();
                tail_time  = typebox.getLatestTime();
                count += drawForward( g, color, insets, coord_xform,
                                      head_time, head_ypos,
                                      tail_time, tail_ypos );
            } 
        }
        else { // OverlapXXclusionRatio
            gap_ypos = ( tail_ypos - head_ypos ) / ( typeboxes.length * 2 );
            for ( idx = typeboxes.length-1; idx >= 0; idx-- ) {
                typebox    = typeboxes[ idx ];
                color      = typebox.getCategoryColor();
                head_time  = typebox.getEarliestTime();
                tail_time  = typebox.getLatestTime();
                count += drawForward( g, color, insets, coord_xform,
                                      head_time, head_ypos,
                                      tail_time, tail_ypos );
                head_ypos += gap_ypos;
                tail_ypos -= gap_ypos;
            } 
        }
        return count;
    }

    public static
    CategoryTimeBox containsPixel( CategoryTimeBox[] typeboxes, Insets insets,
                                   CoordPixelXform coord_xform, Point pt,
                                   float start_ypos, float final_ypos )
    {
        CategoryTimeBox  typebox;
        double           head_time, tail_time;
        float            head_ypos, tail_ypos, gap_ypos;
        int              idx;

        if ( start_ypos < final_ypos ) {
            head_ypos  = start_ypos;
            tail_ypos  = final_ypos;
        }
        else {
            head_ypos  = final_ypos;
            tail_ypos  = start_ypos;
        }

        if ( pt.y < coord_xform.convertRowToPixel( head_ypos ) )
            return null;

        if ( pt.y > coord_xform.convertRowToPixel( tail_ypos ) )
            return null;

        // Search CategoryTimeBox[] in ascending ratio order
        if (    isDisplayTypeEqualWeighted()
             || isDisplayTypeCumulative() ) {
            for ( idx = 0; idx < typeboxes.length; idx++ ) {
                typebox    = typeboxes[ idx ];
                head_time  = typebox.getEarliestTime();
                tail_time  = typebox.getLatestTime();
                if ( isPixelIn( coord_xform, pt,
                                head_time, head_ypos,
                                tail_time, tail_ypos ) )
                    return typebox;
            }
        }
        else { // OverlapXXclusionRatio
            gap_ypos = ( tail_ypos - head_ypos ) / ( typeboxes.length * 2 );
            head_ypos += gap_ypos * (typeboxes.length-1);
            tail_ypos -= gap_ypos * (typeboxes.length-1);
            for ( idx = 0; idx < typeboxes.length; idx++ ) {
                typebox    = typeboxes[ idx ];
                head_time  = typebox.getEarliestTime();
                tail_time  = typebox.getLatestTime();
                if ( isPixelIn( coord_xform, pt,
                                head_time, head_ypos,
                                tail_time, tail_ypos ) )
                    return typebox;
                head_ypos -= gap_ypos;
                tail_ypos += gap_ypos;
            }
        }

        return null;
    }
}
