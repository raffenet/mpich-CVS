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

import base.drawable.Drawable;
import viewer.common.TopWindow;
import viewer.common.Dialogs;
import viewer.common.Parameters;

public class ViewportTimeYaxis extends ViewportTime
                               implements AdjustmentListener
{
    private static final Color    SEARCH_LINE_COLOR       = Color.yellow;
    private static final int      SEARCH_ARROW_HALF_ANGLE = 15;          // deg
    private static final double   SEARCH_ARROW_ANGLE      = Math.PI/6.0; // rad
    private static final double   COS_SEARCH_ARROW_ANGLE
                                  = Math.cos( SEARCH_ARROW_ANGLE );
    private static final double   SIN_SEARCH_ARROW_ANGLE
                                  = Math.sin( SEARCH_ARROW_ANGLE );

    private ModelTime             time_model      = null;
    private YaxisTree             tree_view       = null;

    private Point                 view_pt         = null;
    private ComponentEvent        resize_evt      = null;

    // searchable = view_img is both a Component and ScrollableView object
    private SearchableView        searchable      = null;
    private SearchDialog          search_dialog   = null;

    private Drawable              searched_dobj   = null;
    private double                searching_time;              

    public ViewportTimeYaxis( final ModelTime a_time_model, YaxisTree y_tree )
    {
        super( a_time_model );
        time_model  = a_time_model;
        tree_view   = y_tree;
        view_pt     = new Point( 0, 0 );
        resize_evt  = new ComponentEvent( this,
                                          ComponentEvent.COMPONENT_RESIZED );
        searching_time = time_model.getTimeGlobalMinimum();
        search_dialog  = new SearchDialog( TopWindow.Timeline.getWindow(),
                                           this );
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

               calling the ViewortTime.paint() to avoid redrawing in this class
            */
        super.repaint();
        if ( Debug.isActive() )
            Debug.println( "ViewportTimeYaxis: adjChanged()'s END: " );
    }

    public void fireComponentResized()
    {
        super.componentResized( resize_evt );
    }


    private Rectangle  localRectangleForDrawable( final Drawable dobj )
    {
        Rectangle  dobj_rect;
        Point      local_click; 
        /*
           SwingUtilities.convertPoint() has to be called after the
           CanvasTime has been scrolled, i.e. time_model.scroll().
        */
        dobj_rect = searchable.localRectangleForDrawable( dobj );
        // Update dobj's location to be in ViewportTimeYaxis's coordinate system
        local_click = SwingUtilities.convertPoint( (Component) searchable,
                                                   dobj_rect.x, dobj_rect.y,
                                                   this );
        dobj_rect.setLocation( local_click );
        return dobj_rect;
    }

    private void drawMarkerForSearchedDrawable( Graphics g )
    {
        Stroke     orig_stroke;
        Rectangle  dobj_rect;
        Color      dobj_color, dobj_brighter_color, dobj_darker_color;
        int        vport_width, vport_height;
        int        radius, diameter;
        int        arrow_Xoff, arrow_Yoff;
        int        frame_thickness;
        int        x1, y1, x2, y2, ii;

        vport_width     = this.getWidth();
        vport_height    = this.getHeight();
        dobj_rect       = this.localRectangleForDrawable( searched_dobj );
        // Draw a vertical line along the searched_time;
        if ( dobj_rect.x >= 0 && dobj_rect.x < vport_width ) {
            dobj_color           = searched_dobj.getCategory().getColor();
            dobj_brighter_color  = dobj_color.brighter();
            dobj_darker_color    = dobj_color.darker();

            frame_thickness = Parameters.SEARCH_FRAME_THICKNESS;
            radius          = Parameters.SEARCH_ARROW_LENGTH;
            diameter        = 2 * radius;
            arrow_Xoff      = (int) (radius*SIN_SEARCH_ARROW_ANGLE + 0.5d);
            arrow_Yoff      = (int) (radius*COS_SEARCH_ARROW_ANGLE + 0.5d);

            // Fill upper arrowhead with 2 shades of color
            x1 = dobj_rect.x;
            y1 = dobj_rect.y - frame_thickness;
            g.setColor( dobj_color ); // g.setColor( Color.YELLOW );
            g.fillArc( x1-radius, y1-radius, diameter, diameter,
                       90, -SEARCH_ARROW_HALF_ANGLE );
            g.setColor( dobj_darker_color ); // g.setColor( Color.GRAY );
            g.fillArc( x1-radius, y1-radius, diameter, diameter,
                       90-SEARCH_ARROW_HALF_ANGLE, -SEARCH_ARROW_HALF_ANGLE );
            // Draw upper arrowhead with border
            g.setColor( dobj_brighter_color ); // g.setColor( Color.GRAY );
            g.drawLine( x1, y1, x1, y1-radius );
            g.setColor( dobj_brighter_color ); // g.setColor( Color.WHITE );
            g.drawLine( x1, y1, x1+arrow_Xoff, y1-arrow_Yoff );

            // Fill lower arrowhead with 2 shades of color
            x2 = x1;
            y2 = dobj_rect.y + dobj_rect.height + frame_thickness;
            g.setColor( Color.YELLOW );
            g.fillArc( x2-radius, y2-radius, diameter, diameter,
                       270, SEARCH_ARROW_HALF_ANGLE );
            g.setColor( Color.DARK_GRAY );
            g.fillArc( x2-radius, y2-radius, diameter, diameter,
                       270+SEARCH_ARROW_HALF_ANGLE, SEARCH_ARROW_HALF_ANGLE );
            // Draw lower arrowhead with border
            g.setColor( Color.GRAY );
            g.drawLine( x2, y2, x2, y2+radius );
            g.setColor( Color.WHITE );
            g.drawLine( x2, y2, x2+arrow_Xoff, y2+arrow_Yoff );
        }

        // Compute the intersecting rectangle % the Viewport & the drawable
        dobj_rect = SwingUtilities.computeIntersection(
                                   0, 0, vport_width, vport_height, dobj_rect );
        if ( dobj_rect.x >= 0 ) {
            if ( Parameters.SEARCHED_OBJECT_ON_TOP ) {
                g.setColor( searched_dobj.getCategory().getColor() );
                g.fillRect( dobj_rect.x, dobj_rect.y,
                            dobj_rect.width, dobj_rect.height );
            }
            frame_thickness = Parameters.SEARCH_FRAME_THICKNESS;
            x1  = dobj_rect.x;
            y1  = dobj_rect.y;
            x2  = x1 + dobj_rect.width;
            y2  = y1 + dobj_rect.height;
            // Draw the innermost left & top with a dark color
            g.setColor( Color.BLACK );
                ii = 0;
                g.drawLine( x1-ii, y1-ii, x1-ii, y2+ii );  // left
                g.drawLine( x1-ii, y1-ii, x2+ii, y1-ii );  // top
            // Draw left & top with a bright color
            g.setColor( Color.WHITE );
            for ( ii = 1; ii <= frame_thickness; ii++ ) {
                g.drawLine( x1-ii, y1-ii, x1-ii, y2+ii );  // left
                g.drawLine( x1-ii, y1-ii, x2+ii, y1-ii );  // top
            }
            // Draw the innermost right & bottom with a bright color
            g.setColor( Color.WHITE );
                ii = 0;
                g.drawLine( x2+ii, y1-ii, x2+ii, y2+ii );  // right
                g.drawLine( x1-ii, y2+ii, x2+ii, y2+ii );  // bottom
            // Draw right & bottom with a dark color
            g.setColor( Color.DARK_GRAY );
            for ( ii = 1; ii <= frame_thickness; ii++ ) {
                g.drawLine( x2+ii, y1-ii, x2+ii, y2+ii );  // right
                g.drawLine( x1-ii, y2+ii, x2+ii, y2+ii );  // bottom
            }
        }
    }

    public void paint( Graphics g )
    {
        int   x_pos;

        if ( Debug.isActive() )
            Debug.println( "ViewportTimeYaxis: paint()'s START: " );

        super.paint( g );

        // Draw a line at searching_time
        x_pos = super.coord_xform.convertTimeToPixel( searching_time );
        g.setColor( SEARCH_LINE_COLOR );
        g.drawLine( x_pos, 0, x_pos, this.getHeight() );
        // Draw marker around searched_dobj if it exists
        if ( searched_dobj != null )
            this.drawMarkerForSearchedDrawable( g );

        if ( Debug.isActive() )
            Debug.println( "ViewportTimeYaxis: paint()'s END: " );
    }

    public void eraseSearchedDrawable()
    {
        searched_dobj = null;
    }


    private static final double INVALID_TIME   = Double.NEGATIVE_INFINITY;
    private              double searched_time  = INVALID_TIME;

    /*
        searchBackward() is for ActionSearchBackward
    */
    public boolean searchBackward()
    {
        InfoPanelForDrawable  dobj_panel = null;

        if ( searching_time != searched_time )
            dobj_panel  = (InfoPanelForDrawable)
                          searchable.searchPreviousComponent( searching_time );
        else
            dobj_panel  = (InfoPanelForDrawable)
                          searchable.searchPreviousComponent();
        if ( dobj_panel != null ) {
            searched_dobj = dobj_panel.getDrawable();
            searched_time = searched_dobj.getEarliestTime();
            // Scroll the Time axis and set Time Focus at the drawable found.
            time_model.scroll( searched_time - searching_time );
            searching_time = searched_time;
            // Scroll the Y-axis as well so searched_dobj becomes visible
            tree_view.scrollRowToVisible( searched_dobj.getRowID() );
            //  call this.paint( g );
            this.repaint();

            search_dialog.replace( dobj_panel );
            if ( ! search_dialog.isVisible() )
                 search_dialog.setVisibleAtDefaultLocation();
            return true;
        }
        else {
            Frame top_frame = (Frame) SwingUtilities.windowForComponent( this );
            if (    searched_dobj != null
                 && (    searched_dobj.getEarliestTime()
                      == time_model.getTimeGlobalMinimum() ) )
                Dialogs.info( top_frame,
                              "The FIRST drawable in the logfile has been "
                            + "reached.\n  Search backward has no more "
                            + "drawable to return.\n", null );
            else
                Dialogs.warn( top_frame,
                              "If the logfile's beginning is not within view,\n"
                            + "SCROLL BACKWARD till you see more drawables\n"
                            + "are within view.  All drawables in view or in \n"
                            + "the memory have been searched.\n" );
            search_dialog.setVisible( false );
            searched_dobj = null;
            searched_time = INVALID_TIME;
            this.repaint();
            return false;
        }
    }

    /*
        searchForward() is for ActionSearchForward
    */
    public boolean searchForward()
    {
        InfoPanelForDrawable  dobj_panel = null;

        if ( searching_time != searched_time )
            dobj_panel  = (InfoPanelForDrawable)
                          searchable.searchNextComponent( searching_time );
        else
            dobj_panel  = (InfoPanelForDrawable)
                          searchable.searchNextComponent();

        if ( dobj_panel != null ) {
            searched_dobj = dobj_panel.getDrawable();
            searched_time = searched_dobj.getEarliestTime();
            // Scroll the screen and set Time Focus at the drawable found.
            time_model.scroll( searched_time - searching_time );
            searching_time = searched_time;
            // Scroll the Y-axis as well so searched_dobj becomes visible
            tree_view.scrollRowToVisible( searched_dobj.getRowID() );
            //  call this.paint( g );
            this.repaint();

            search_dialog.replace( dobj_panel );
            if ( ! search_dialog.isVisible() )
                 search_dialog.setVisibleAtDefaultLocation();
            return true;
        }
        else {
            Frame top_frame = (Frame) SwingUtilities.windowForComponent( this );
            if (    searched_dobj != null
                 && (    searched_dobj.getLatestTime()
                      == time_model.getTimeGlobalMaximum() ) )
                Dialogs.info( top_frame,
                              "The LAST drawable in the logfile has been "
                            + "reached.\n  Search forward has no more "
                            + "drawable to return.\n", null );
            else
                Dialogs.warn( top_frame,
                              "If the end of the logfile is not within view,\n"
                            + "SCROLL FORWARD till you see more drawables\n"
                            + "are within view.  All drawables in view or in \n"
                            + "the memory have been searched.\n" );
            search_dialog.setVisible( false );
            searched_dobj = null;
            searched_time = INVALID_TIME;
            this.repaint();
            return false;
        }
    }

    public boolean searchInit()
    {
        InfoDialog  info_popup = super.getLastInfoDialog();
        if ( info_popup != null ) {
            searching_time = info_popup.getClickedTime();
            info_popup.getCloseButton().doClick();
            return true;
        }
        else {
            Frame top_frame = (Frame) SwingUtilities.windowForComponent( this );
            Dialogs.warn( top_frame,
                          "No info dialog box! Info dialog box can be set\n"
                        + "by right mouse clicking on the timeline canvas\n" );
            return false;
        }
    }

        /*
            Interface to Overload MouseInputListener()
        */
        public void mouseClicked( MouseEvent mouse_evt )
        {
            Point  vport_click;
            if (    mouse_evt.isControlDown()
                 || SwingUtilities.isMiddleMouseButton( mouse_evt ) ) {
                vport_click    = mouse_evt.getPoint();
                searching_time = super.coord_xform.convertPixelToTime(
                                                   vport_click.x );
                this.repaint();
            }
        }
}
