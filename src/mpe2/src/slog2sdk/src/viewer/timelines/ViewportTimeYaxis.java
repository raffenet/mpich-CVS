/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class ViewportTimeYaxis extends ViewportTime
                               implements AdjustmentListener
{
    private Point                     view_pt;
    private ComponentEvent            resize_evt;

    public ViewportTimeYaxis( final ModelTime time_model )
    {
        super( time_model );
        view_pt    = new Point( 0, 0 );
        resize_evt = new ComponentEvent( this,
                                         ComponentEvent.COMPONENT_RESIZED );
    }

    public void adjustmentValueChanged( AdjustmentEvent evt )
    {
        if ( Debug.isActive() ) {
            Debug.println( "ViewportTimeYaxis: adjChanged()'s START: " );
            Debug.println( "adj_evt = " + evt );
        }
        view_pt.x  = super.getXaxisViewPosition();
        view_pt.y  = evt.getValue();
        super.setYaxisViewPosition( view_pt.y );
        super.setViewPosition( view_pt );
            /*
               calling view.repaint() to ensure the view is repainted
               after setViewPosition is called.
               -- apparently, super.repaint(), the RepaintManager, has invoked 
                  ( (Component) view_img ).repaint();
               -- JViewport.setViewPosition() may have invoked super.repaint()
            */
        super.repaint();
        if ( Debug.isActive() )
            Debug.println( "ViewportTimeYaxis: adjChanged()'s END: " );
    }

    public void fireComponentResized()
    {
        super.componentResized( resize_evt );
    }
}
