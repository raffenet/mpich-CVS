/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.text.NumberFormat;
import java.text.DecimalFormat;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

import base.drawable.TimeBoundingBox;
import viewer.common.Const;

public class RulerTime extends ScrollableObject
{
    private static final   Font  FONT              = Const.FONT;
    private static final   int   FONT_SIZE         = FONT.getSize();
    private static final   int   TICKMARK_HEIGHT   = 10;
    private static final   int   I_FONT_BASELINE   = TICKMARK_HEIGHT
                                                   + FONT_SIZE + 5;
    private static final   int   VIEW_HEIGHT       = I_FONT_BASELINE + 5;

    private double         tRange;
    private double         tIncrement;
    private DecimalFormat  fmt;

    public RulerTime( ModelTime model )
    {
        super( model );
        fmt = (DecimalFormat) NumberFormat.getInstance();
        fmt.applyPattern( Const.RULER_TIME_FORMAT );
    }

    public Dimension getMinimumSize()
    {
        //  the width below is arbitary
        Debug.println( "RulerTime: min_size = "
                     + "(0," + VIEW_HEIGHT + ")" );
        return new Dimension( 0, VIEW_HEIGHT );
    }

    public Dimension getMaximumSize()
    {
        Debug.println( "RulerTime: max_size = "
                     + "(" + Short.MAX_VALUE + "," + VIEW_HEIGHT + ")" );
        return new Dimension( Short.MAX_VALUE, VIEW_HEIGHT );
    }

    //  Function defined the height of the JComponent.
    public int getJComponentHeight()
    {
        return VIEW_HEIGHT;
    }

    protected void initializeAllOffImages( final TimeBoundingBox imgs_times )
    {}

    protected void finalizeAllOffImages( final TimeBoundingBox imgs_times )
    {}

    protected void drawOneOffImage(       Image            offImage,
                                   final TimeBoundingBox  timebounds )
    {
        Debug.println( "RulerTime: drawOneOffImage()'s offImage = "
                     + offImage );
        if ( offImage != null ) {
            // int offImage_width = visible_size.width * NumViewsPerImage;
            int offImage_width   = offImage.getWidth( this );
            int offImage_height  = offImage.getHeight( this ); 
            // int offImage_height  = VIEW_HEIGHT; 
            Graphics offGraphics = offImage.getGraphics();

            // offGraphics.getClipBounds() returns null
            // offGraphics.setClip( 0, 0, getWidth()/NumImages, getHeight() );
            // Do the ruler labels in a small font that's black.
            offGraphics.setColor( Color.white );
            offGraphics.fillRect( 0, 0, offImage_width, offImage_height );
            offGraphics.setFont( FONT );
            offGraphics.setColor( Color.black );

            if ( timebounds.getLength() != tRange ) {
                tRange     = timebounds.getLength();
                tIncrement = tRange / ( NumViewsPerImage * 10.0 );
                tIncrement = ModelTime.getRulerIncrement( tIncrement );
            }

            double time, tInitMark, tFinalMark;
            int    i_X, i_X_0;
            String text = null;

            Debug.print( "RulerTime.drawOffImage at : " );
            double t_init = timebounds.getEarliestTime();
            i_X_0 = super.time2pixel( t_init );
            tInitMark  = ModelTime.getRulerFirstMark( t_init, tIncrement );
            tFinalMark = timebounds.getLatestTime() + tIncrement;
            for ( time = tInitMark; time < tFinalMark; time += tIncrement ) {
                i_X = super.time2pixel( time ) - i_X_0;
                // offGraphics.drawLine( i_X, offImage_height-1,
                //                       i_X, 
                //                       offImage_height-TICKMARK_HEIGHT-1 );
                offGraphics.drawLine( i_X, 1, i_X, TICKMARK_HEIGHT );
                text = fmt.format( time );
                offGraphics.drawString( text, i_X - 3, I_FONT_BASELINE );
                Debug.print( time + ":" + i_X + ", " ); 
            }
            Debug.println( "|" );

            offGraphics.dispose();
        }
    }

}
