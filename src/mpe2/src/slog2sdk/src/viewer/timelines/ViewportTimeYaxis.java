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

import viewer.common.TopWindow;
import viewer.common.Dialogs;

public class ViewportTimeYaxis extends ViewportTime
                               implements AdjustmentListener
{
    private Point                     view_pt;
    private ComponentEvent            resize_evt;

    // searchable = view_img is both a Component and ScrollableView object
    private SearchableView            searchable    = null;
    private ModelTime                 time_model    = null;

    public ViewportTimeYaxis( final ModelTime a_time_model )
    {
        super( a_time_model );
        time_model = a_time_model;
        view_pt    = new Point( 0, 0 );
        resize_evt = new ComponentEvent( this,
                                         ComponentEvent.COMPONENT_RESIZED );
    }

    public void setView( Component view )
    {
        super.setView( view );
        // causes exception if view_img is not SearchableView
        searchable  = (SearchableView) view;
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


    private static final double INVALID_TIME = Double.NEGATIVE_INFINITY;
    private              double marked_time  = INVALID_TIME;

    /*
        searchBackward() is for ActionSearchBackward
    */
    public Component searchBackward()
    {
        InfoPanelForDrawable  dobj_panel;
        boolean               isNewSearch;
        double                focus_time;

        focus_time  = time_model.getTimeZoomFocus();
        isNewSearch = (focus_time != marked_time);
        dobj_panel  = (InfoPanelForDrawable)
                      searchable.searchPreviousComponent( isNewSearch );
        if ( dobj_panel != null ) {
            marked_time = dobj_panel.getDrawable().getEarliestTime();
            // Scroll the screen and set Time Focus at the drawable found.
            time_model.scroll( marked_time - focus_time );
            time_model.setTimeZoomFocus( marked_time );
            super.zoom_timebox.setZeroDuration( marked_time );
            super.repaint();
        }
        else
            Dialogs.warn( TopWindow.Timeline.getWindow(),
                          "Most likely the First drawable in the logfile\n"
                        + "has been reached or an error has just occured.\n" );
        return dobj_panel;
    }

    /*
        searchForward() is for ActionSearchForward
    */
    public Component searchForward()
    {
        InfoPanelForDrawable  dobj_panel;
        boolean               isNewSearch;
        double                focus_time;

        focus_time  = time_model.getTimeZoomFocus();
        isNewSearch = (focus_time != marked_time);
        dobj_panel  = (InfoPanelForDrawable)
                      searchable.searchNextComponent( isNewSearch );
        if ( dobj_panel != null ) {
            marked_time = dobj_panel.getDrawable().getEarliestTime();
            // Scroll the screen and set Time Focus at the drawable found.
            time_model.scroll( marked_time - focus_time );
            time_model.setTimeZoomFocus( marked_time );
            super.zoom_timebox.setZeroDuration( marked_time );
            super.repaint();
        }
        else
            Dialogs.warn( TopWindow.Timeline.getWindow(),
                          "Most likely the LAST drawable in the logfile\n"
                        + "has been reached or an error has just occured.\n" );
        return dobj_panel;
    }
}
