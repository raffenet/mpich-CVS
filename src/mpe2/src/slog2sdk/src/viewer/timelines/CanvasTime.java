/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.util.Date;
import java.util.Map;
import java.util.Iterator;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;
import base.drawable.Shadow;
import base.drawable.NestingStacks;
import base.drawable.DrawnBoxSet;
import logformat.slog2.input.TreeNode;
import logformat.slog2.input.TreeTrunk;
import viewer.common.Dialogs;
import viewer.common.Routines;
import viewer.common.TopWindow;
import viewer.common.Parameters;

public class CanvasTime extends ScrollableObject
{
    private static final int   MIN_VISIBLE_ROW_COUNT = 2;

    private Component          root_window;
    private TreeTrunk          treetrunk;
    private YaxisMaps          y_maps;
    private YaxisTree          tree_view;
    private BoundedRangeModel  y_model;
    private TimeBoundingBox    timeframe4imgs;   // TimeFrame for images[]

    private ChangeListener     change_listener;
    private ChangeEvent        change_event;

    private int                num_rows;
    private int                row_height;
    private NestingStacks      nesting_stacks;
    private Map                map_line2row;
    private DrawnBoxSet        drawn_boxes;

    private Date               init_time, final_time;


    public CanvasTime( Component parent,
                       ModelTime time_model, TreeTrunk treebody,
                       BoundedRangeModel yaxis_model, YaxisMaps in_maps )
    {
        super( time_model );

        TreeNode   treeroot;
        short      depth_max, depth_init;

        root_window     = parent;
        treetrunk       = treebody;
        y_maps          = in_maps;
        tree_view       = y_maps.getTreeView();
        y_model         = yaxis_model;
        treeroot        = treetrunk.getTreeRoot();
        depth_max       = treeroot.getTreeNodeID().depth;
        nesting_stacks  = new NestingStacks( tree_view );
        map_line2row    = null;
        drawn_boxes     = new DrawnBoxSet( tree_view );
        // timeframe4imgs to be initialized later in initializeAllOffImages()
        timeframe4imgs  = null;

        depth_init      = (short) ( depth_max
                                  - Parameters.INIT_SLOG2_LEVEL_READ + 1 );
        if ( depth_init < 0 )
            depth_init = 0;
        treetrunk.growInTreeWindow( treeroot, depth_init,
                                    new TimeBoundingBox( treeroot ) );

        change_event    = null;
        change_listener = null;

        CanvasMouseListener mouse_listener;
        mouse_listener = new CanvasMouseListener( (Frame) root_window,
                                                  this, time_model );
        addMouseListener( mouse_listener );
        addMouseMotionListener( mouse_listener );
    }

    public void addChangeListener( ChangeListener listener )
    {
        change_event    = new ChangeEvent( this );
        change_listener = listener; 
    }

    public Dimension getMinimumSize()
    {
        int  min_view_height = MIN_VISIBLE_ROW_COUNT
                             * Parameters.Y_AXIS_ROW_HEIGHT;
        //  the width below is arbitary
        Debug.println( "CanvasTime: min_size = "
                     + "(0," + min_view_height + ")" );
        return new Dimension( 0, min_view_height );
    }

    public Dimension getMaximumSize()
    {
        Debug.println( "CanvasTime: max_size = "
                     + "(" + Short.MAX_VALUE + "," + Short.MAX_VALUE + ")" );
        return new Dimension( Short.MAX_VALUE, Short.MAX_VALUE );
    }

    public int getJComponentHeight()
    {
        int rows_size = tree_view.getRowCount() * tree_view.getRowHeight();
        int view_size = y_model.getMaximum() - y_model.getMinimum() + 1;
        if ( view_size > rows_size )
            return view_size;
        else
            return rows_size;
    }

    private void fireChangeEvent()
    {
        if ( change_event != null )
            change_listener.stateChanged( change_event );
    }

    protected void initializeAllOffImages( final TimeBoundingBox imgs_times )
    {
        init_time = new Date();
        // Read the SLOG-2 TreeNodes within TimeFrame into memory
        Routines.setAllCursorsToWait( root_window );
        if ( timeframe4imgs == null )
            timeframe4imgs = new TimeBoundingBox( imgs_times );
        treetrunk.updateTimeWindow( timeframe4imgs, imgs_times );
        num_rows         = tree_view.getRowCount();
        row_height       = tree_view.getRowHeight();
        nesting_stacks.initialize();

        map_line2row = y_maps.getMapOfLineIDToRowID();
        if ( map_line2row == null ) {
            if ( ! y_maps.update() )
                Dialogs.error( TopWindow.Timeline.getWindow(),
                               "Error in updating YaxisMaps!" );
            map_line2row = y_maps.getMapOfLineIDToRowID();
        }
        // System.out.println( "map_line2row = " + map_line2row );
        drawn_boxes.initialize();
    }

    protected void finalizeAllOffImages( final TimeBoundingBox imgs_times )
    {
        drawn_boxes.finalize();
        map_line2row = null;        
        nesting_stacks.finalize();
        // Update the timeframe of all images
        timeframe4imgs.setEarliestTime( imgs_times.getEarliestTime() );
        timeframe4imgs.setLatestTime( imgs_times.getLatestTime() );
        this.fireChangeEvent();  // to update TreeTrunkPanel.
        Routines.setAllCursorsToNormal( root_window );

        final_time = new Date();
        System.out.println( "drawOffImages: time = "
                          + (final_time.getTime()-init_time.getTime()) );
    }

    protected void drawOneOffImage(       Image            offImage,
                                    final TimeBoundingBox  timebounds )
    {
        Debug.println( "CanvasTime: drawOneOffImage()'s offImage = "
                     + offImage );
        if ( offImage != null ) {
            // int offImage_width = visible_size.width * NumViewsPerImage;
            int        offImage_width  = offImage.getWidth( this );
            int        offImage_height = offImage.getHeight( this ); 
            Graphics2D offGraphics     = (Graphics2D) offImage.getGraphics();

            // Set AntiAliasing On
            offGraphics.setRenderingHint( RenderingHints.KEY_ANTIALIASING,
                                          RenderingHints.VALUE_ANTIALIAS_ON );
            offGraphics.setRenderingHint( RenderingHints.KEY_RENDERING,
                                          RenderingHints.VALUE_RENDER_SPEED );

            // offGraphics.getClipBounds() returns null
            // offGraphics.setClip( 0, 0, getWidth()/NumImages, getHeight() );
            // Do the ruler labels in a small font that's black.
            offGraphics.setColor( Color.black );
            offGraphics.fillRect( 0, 0, offImage_width, offImage_height );

            int    irow;
            int    i_Y;

            CoordPixelImage coord_xform;
            coord_xform = new CoordPixelImage( this, row_height, timebounds );

            // Draw the center TimeLines.
            offGraphics.setColor( Color.red );
            for ( irow = 0 ; irow < num_rows ; irow++ ) {
                //  Select only non-expanded row
                if ( ! tree_view.isExpanded( irow ) ) {
                    i_Y = coord_xform.convertRowToPixel( (float) irow );
                    offGraphics.drawLine( 0, i_Y, offImage_width-1, i_Y );
                }
                else {
                    i_Y = coord_xform.convertRowToPixel( (float) irow );
                    offGraphics.drawLine( 0, i_Y-row_height/2,
                                          0, i_Y+row_height/2 );
                }
                 
            }

            nesting_stacks.reset();
            drawn_boxes.reset();


            Iterator sobjs;
            Shadow   sobj;
            Iterator dobjs;
            Drawable dobj;

            int N_nestable = 0, N_nestless = 0;
            int N_nestable_drawn = 0, N_nestless_drawn = 0;

            // Draw Nestable Shadows
            sobjs = treetrunk.iteratorOfLowestFloorShadows( timebounds,
                                                            true, true );
            while ( sobjs.hasNext() ) {
                sobj = (Shadow) sobjs.next();
                N_nestable_drawn +=
                sobj.drawOnCanvas( offGraphics, coord_xform, map_line2row,
                                   drawn_boxes, nesting_stacks );
                N_nestable++;
            }
            
            // Draw Nestable Drawables
            dobjs = treetrunk.iteratorOfDrawables( timebounds, true, true );
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next();
                N_nestable_drawn +=
                dobj.drawOnCanvas( offGraphics, coord_xform, map_line2row,
                                   drawn_boxes, nesting_stacks );
                N_nestable++;
            }

            // Draw Nestless Shadows
            sobjs = treetrunk.iteratorOfLowestFloorShadows( timebounds,
                                                            true, false );
            while ( sobjs.hasNext() ) {
                sobj = (Shadow) sobjs.next();
                N_nestless_drawn +=
                sobj.drawOnCanvas( offGraphics, coord_xform, map_line2row,
                                   drawn_boxes, nesting_stacks );
                N_nestless++;
            }

            // Draw Nestless Drawables
            dobjs = treetrunk.iteratorOfDrawables( timebounds, true, false );
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next(); 
                N_nestless_drawn +=
                dobj.drawOnCanvas( offGraphics, coord_xform, map_line2row,
                                   drawn_boxes, nesting_stacks );
                N_nestless++;
            }

            System.out.println( "CanvasTime.drawOneOffImage(): N_nestable = "
                              + N_nestable_drawn + "/" + N_nestable
                              + ",  N_nestless = "
                              + N_nestless_drawn + "/" + N_nestless );

            // System.out.println( treetrunk.toStubString() );
            offGraphics.dispose();
        }
    }

    private class CanvasMouseListener extends MouseInputAdapter
    {
        private Frame              root_window;
        private ScrollableObject   scrollable;
        private ModelTime          time_model;

        public CanvasMouseListener( final Frame             in_root_window,
                                    final ScrollableObject  in_scrollable,
                                    final ModelTime         in_model )
        {
            root_window  = in_root_window;
            scrollable   = in_scrollable;
            time_model   = in_model;
        }

        private Point getGlobalClickPoint( final Point local_click )
        {
            Point origin       = scrollable.getLocationOnScreen();
            return   new Point( origin.x + local_click.x,
                                origin.y + local_click.y );
        }

        public void mouseClicked( MouseEvent mevt )
        {
            if ( SwingUtilities.isRightMouseButton( mevt ) )
                this.rightMouseClicked( mevt );
            else if ( SwingUtilities.isLeftMouseButton( mevt ) )
                this.leftMouseClicked( mevt );
        }

        private void rightMouseClicked( MouseEvent mevt )
        {
            DrawableInfoDialog  dobj_dialog;
            Point               local_click, global_click;

            local_click = mevt.getPoint();
            // System.out.println( "\nrightMouseClicked at " + local_click );
            CoordPixelImage coord_xform;
            coord_xform = new CoordPixelImage( scrollable, row_height, 
                                        scrollable.getTimeBoundsOfImages() );

            // Determine the timeframe of the current view
            TimeBoundingBox timeframe = new TimeBoundingBox();
            timeframe.setEarliestTime( time_model.getTimeViewPosition() );
            timeframe.setLatestFromEarliest( time_model.getTimeViewExtent() );
            // System.out.println( "CurrView's timeframe = " + timeframe );

            Map map_line2row = y_maps.getMapOfLineIDToRowID();
            if ( map_line2row == null ) {
                if ( ! y_maps.update() )
                    Dialogs.error( TopWindow.Timeline.getWindow(),
                                   "Error in updating YaxisMaps!" );
                map_line2row = y_maps.getMapOfLineIDToRowID();
            }

            Iterator sobjs;
            Shadow   sobj;
            Iterator dobjs;
            Drawable dobj;
            Drawable clicked_dobj;

            // Search Nestless Drawables
            dobjs = treetrunk.iteratorOfDrawables( timeframe, false, false );
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next();
                clicked_dobj = dobj.getDrawableWithPixel( coord_xform,
                                                          map_line2row,
                                                          local_click );
                if ( clicked_dobj != null ) {
                    global_click = getGlobalClickPoint( local_click );
                    dobj_dialog  = new DrawableInfoDialog( root_window,
                                                           clicked_dobj );
                    dobj_dialog.setVisibleAtLocation( global_click );
                    return;
                }
            }

            // Search Nestless Shadows
            sobjs = treetrunk.iteratorOfLowestFloorShadows( timeframe,
                                                            false, false );
            while ( sobjs.hasNext() ) {
                sobj = (Shadow) sobjs.next();
                clicked_dobj = sobj.getDrawableWithPixel( coord_xform,
                                                          map_line2row,
                                                          local_click );
                if ( clicked_dobj != null ) {
                    global_click = getGlobalClickPoint( local_click );
                    dobj_dialog  = new DrawableInfoDialog( root_window,
                                                           clicked_dobj );
                    dobj_dialog.setVisibleAtLocation( global_click );
                    return;
                }
            }

            // Search Nestable Drawables
            dobjs = treetrunk.iteratorOfDrawables( timeframe, false, true );
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next();
                clicked_dobj = dobj.getDrawableWithPixel( coord_xform,
                                                          map_line2row,
                                                          local_click );
                if ( clicked_dobj != null ) {
                    global_click = getGlobalClickPoint( local_click );
                    dobj_dialog  = new DrawableInfoDialog( root_window,
                                                           clicked_dobj );
                    dobj_dialog.setVisibleAtLocation( global_click );
                    return;
                }
            }
            
            // Search Nestable Shadows
            sobjs = treetrunk.iteratorOfLowestFloorShadows( timeframe,
                                                            false, true );
            while ( sobjs.hasNext() ) {
                sobj = (Shadow) sobjs.next();
                clicked_dobj = sobj.getDrawableWithPixel( coord_xform,
                                                          map_line2row,
                                                          local_click );
                if ( clicked_dobj != null ) {
                    global_click = getGlobalClickPoint( local_click );
                    dobj_dialog  = new DrawableInfoDialog( root_window,
                                                           clicked_dobj );
                    dobj_dialog.setVisibleAtLocation( global_click );
                    return;
                }
            }
        }

        private void leftMouseClicked( MouseEvent mevt )
        {
            Point               local_click;
            double              clicked_time;

            local_click = mevt.getPoint();
            CoordPixelImage coord_xform;
            coord_xform = new CoordPixelImage( scrollable, row_height,
                                        scrollable.getTimeBoundsOfImages() );
            clicked_time = coord_xform.convertPixelToTime( local_click.x );
            time_model.setTimeZoomFocus( clicked_time );    
            // System.out.println( "\nleftMouseClicked at " + local_click
            //                   + ".  i.e at time " + clicked_time );
        }

        /*
        public void mouseReleased( MouseEvent mevt )
        {
            System.out.println( "mouseReleased at " + mevt.getPoint() );
        }
        */
    }
}
