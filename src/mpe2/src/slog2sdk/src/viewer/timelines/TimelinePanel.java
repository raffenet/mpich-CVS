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
import javax.swing.border.*;
import javax.swing.event.*;

import logformat.slog2.LineIDMapList;
import logformat.slog2.LineIDMap;
import logformat.slog2.input.InputLog;
import logformat.slog2.input.TreeTrunk;
import logformat.slog2.input.TreeNode;
import viewer.common.Const;

public class TimelinePanel extends JPanel
{
    private InputLog                slog_ins;
    private TreeTrunk               treetrunk;
    private TreeNode                treeroot;

    private TimelineToolBar         toolbar;
    private TreeTrunkPanel          treetrunk_panel;
    private BoundedRangeModel       y_model;
    private YaxisMaps               y_maps;
    private YaxisTree               y_tree;
    private JScrollPane             y_scroller;
    private JScrollBar              y_scrollbar;


    private ModelTime               time_model;
    private ScrollbarTime           time_scrollbar;
    private ModelTimePanel          time_display_panel;

    private RulerTime               time_ruler;
    private ViewportTime            time_ruler_vport;
    private ViewportTimePanel       time_ruler_panel;

    private CanvasTime              time_canvas;
    private ViewportTimeYaxis       time_canvas_vport;
    private ViewportTimePanel       time_canvas_panel;



    public TimelinePanel( final InputLog in_slog, int view_ID )
    {
        super();
        slog_ins     = in_slog;

        /*
        System.out.println( "ScrollBar.MinThumbSize = "
                          + UIManager.get( "ScrollBar.minimumThumbSize" ) );
        System.out.println( "ScrollBar.MaxThumbSize = "
                          + UIManager.get( "ScrollBar.maximumThumbSize" ) );
        */
        Dimension sb_minThumbSz = (Dimension)
                                  UIManager.get( "ScrollBar.minimumThumbSize" );        sb_minThumbSz.width = 4;
        UIManager.put( "ScrollBar.minimumThumbSize", sb_minThumbSz );


        /* Initialize the YaxisMaps through the initialization of YaxisTree */
        LineIDMapList lineIDmaps = slog_ins.getLineIDMapList();
        LineIDMap     lineIDmap  = (LineIDMap) lineIDmaps.get( view_ID );
        String[]      y_colnames  = lineIDmap.getColumnLabels();
        y_maps      = new YaxisMaps( lineIDmap );
        y_tree      = new YaxisTree( y_maps.getTreeRoot() );
        y_maps.setTreeView( y_tree );   
                    /* done YaxisMaps initialization */
        /*
           y_scroller for y_tree needs to be created before time_canvas, so
           y_model can be extracted to be used for the creation of time_canvas
        */
        y_scroller  = new JScrollPane( y_tree, 
                          ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS,
                          ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS );
        y_scrollbar = y_scroller.getVerticalScrollBar();
        y_model     = y_scrollbar.getModel();

        /* Initialize the ModelTime slog.input.InputLog().getTreeRoot() */
        treetrunk     = new TreeTrunk( slog_ins );
        treetrunk.initFromTreeTop();
        treeroot      = treetrunk.getTreeRoot();
        time_model    = new ModelTime( treeroot.getEarliestTime(), 
                                       treeroot.getLatestTime() );
        time_model.setTimeZoomFactor(
                   (double) slog_ins.getNumChildrenPerNode() );
        System.out.println( "slog_ins.tZoomFtr = "
                          + time_model.getTimeZoomFactor() );

        this.setLayout( new BorderLayout() );

            /* Setting up the RIGHT panel to store various time-related GUIs */
            JPanel right_panel = new JPanel();
            right_panel.setLayout( new BoxLayout( right_panel,
                                                  BoxLayout.Y_AXIS ) );

                /* The View's Time Display Panel */
                time_display_panel = new ModelTimePanel( time_model );
                time_model.setParamDisplay( time_display_panel );

                /* The Time Ruler */
                time_ruler        = new RulerTime( time_model );
                time_ruler_vport  = new ViewportTime( time_model );
                time_ruler_vport.setView( time_ruler );
                time_ruler_panel  = new ViewportTimePanel( time_ruler_vport );
                time_ruler_panel.setBorderTitle( " Time (seconds) ",
                                                 TitledBorder.RIGHT,
                                                 TitledBorder.BOTTOM,
                                                 Const.FONT, Color.red ); 
                /*
                   Propagation of AdjustmentEvent originating from scroller:

                   scroller -----> time_model -----> viewport -----> view
                             adj               time           paint
                   viewport is between time_model and view because
                   viewport is what user sees.  
                */
                time_model.addTimeListener( time_ruler_vport );
                /*
                   Since there is NOT a specific ViewportTime/ViewTimePanel
                   for RulerTime, so we need to set PreferredSize of RulerTime
                   here.  Since CanvasTime's has its MaximumSize set to MAX,
                   CanvasTime's ViewportTimePanel will become space hungary.
                   As we want RulerTime to be fixed height during resize
                   of the top level window, So it becomes CRUCIAL to set 
                   Preferred Height of RulerTime's ViewportTimePanel equal
                   to its Minimum Height and Maximum Height.
                */
                Insets   ruler_panel_insets = time_ruler_panel.getInsets();
                int      ruler_panel_height = ruler_panel_insets.top
                                            + time_ruler.getJComponentHeight()
                                            + ruler_panel_insets.bottom;
                time_ruler_panel.setPreferredSize(
                     new Dimension( 100, ruler_panel_height ) );

                /* The TimeLine Canvas */
                time_canvas       = new CanvasTime( time_model, treetrunk,
                                                    y_model, y_maps,
                                                    y_colnames );
                time_canvas_vport = new ViewportTimeYaxis( time_model );
                time_canvas_vport.setView( time_canvas );
                time_canvas_panel = new ViewportTimePanel( time_canvas_vport );
                time_canvas_panel.setBorderTitle( " TimeLines ",
                                                  TitledBorder.RIGHT,
                                                  TitledBorder.TOP,
                                                  null, Color.blue );
                /* Inform "time_canvas_vport" time has been changed */
                time_model.addTimeListener( time_canvas_vport );

                /* The Horizontal "Time" ScrollBar */
                time_scrollbar = new ScrollbarTime( time_model );
                time_scrollbar.setEnabled( true );
                time_model.setScrollBar( time_scrollbar );

            right_panel.add( time_display_panel );
            right_panel.add( time_canvas_panel );
            right_panel.add( time_scrollbar );
            right_panel.add( time_ruler_panel );

            /* Setting up the LEFT panel to store various Y-axis related GUIs */
            JPanel left_panel = new JPanel();
            left_panel.setLayout( new BoxLayout( left_panel,
                                                 BoxLayout.Y_AXIS ) );

                // SLOG-2 Tree's Depth Panel
                treetrunk_panel = new TreeTrunkPanel( treetrunk );
                treetrunk_panel.setAlignmentX( Component.CENTER_ALIGNMENT );
                treetrunk_panel.setAlignmentY( Component.CENTER_ALIGNMENT );
                /* Inform "treetrunk_panel" lowest-depth has been changed  */
                time_canvas.addChangeListener( treetrunk_panel );

                /* "VIEW" title */
                Insets canvas_panel_insets = time_canvas_panel.getInsets();
                JLabel y_title_top = new JLabel( lineIDmap.getTitle() );
                y_title_top.setPreferredSize(
                               new Dimension( 20, canvas_panel_insets.top ) );
                y_title_top.setBorder( BorderFactory.createEtchedBorder() );
                y_title_top.setAlignmentX( Component.CENTER_ALIGNMENT );
                y_title_top.setAlignmentY( Component.CENTER_ALIGNMENT );

                /* YaxisTree View for SLOG-2 */
                y_scroller.setAlignmentX( Component.CENTER_ALIGNMENT );
                y_scroller.setAlignmentY( Component.CENTER_ALIGNMENT );
                /* when y_scrollbar is changed, update time_canvas as well. */
                y_scrollbar.addAdjustmentListener( time_canvas_vport );

                /* YaxisTree's Column Labels */
                int       left_bottom_height = ruler_panel_height
                                             + canvas_panel_insets.bottom;
                JTextArea y_colarea   = new JTextArea();
                // y_colarea.setFont( Const.FONT );
                StringBuffer text_space  = new StringBuffer( " " );
                for ( int idx = 0; idx < y_colnames.length; idx++ ) {
                    y_colarea.append( text_space.toString() + "@ "
                                    + y_colnames[ idx ] + "\n" );
                    text_space.append( "    " );
                }
                JScrollPane y_colpanel = new JScrollPane( y_colarea,
                          ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS,
                          ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS );
                /*
                   Since there is NOT a specific Top Level JPanel for 
                   y_colpanel, so we need to set its PreferredSize here.
                   Since y_scroller(i.e. JScrollPane containing YaxisTree)
                   is the space hungary component here.  So it is CRUCIAL 
                   to set the height PreferredSize equal to that of MinimumSize
                   and MaximumSize, hence y_colpanel will be fixed in height
                   during resizing of the top level frame.
                */
                y_colpanel.setMinimumSize(
                     new Dimension( 0, left_bottom_height ) );
                y_colpanel.setMaximumSize(
                     new Dimension( Short.MAX_VALUE, left_bottom_height ) );
                y_colpanel.setPreferredSize(
                     new Dimension( 20, left_bottom_height ) );
                // JLabel y_title_btm = new JLabel( "       " );
                // y_title_btm.setPreferredSize(
                //                new Dimension( 10, left_bottom_height ) );
                // y_title_btm.setBorder( BorderFactory.createEtchedBorder() );
                // y_title_btm.setAlignmentX( Component.CENTER_ALIGNMENT );
                // y_title_btm.setAlignmentY( Component.CENTER_ALIGNMENT );

            left_panel.add( treetrunk_panel );
            left_panel.add( y_title_top );
            left_panel.add( y_scroller );
            // left_panel.add( y_title_btm );
            left_panel.add( y_colpanel );

            /* Store the LEFT and RIGHT panels in the JSplitPane */
            JSplitPane splitter;
            splitter = new JSplitPane( JSplitPane.HORIZONTAL_SPLIT,
                                       false, left_panel, right_panel );
            splitter.setOneTouchExpandable( true );

        this.add( splitter, BorderLayout.CENTER );

            /* The ToolBar for various user controls */
            toolbar = new TimelineToolBar( time_canvas_vport,
                                           y_scrollbar, y_tree, y_maps,
                                           time_scrollbar, time_model );

        this.add( toolbar, BorderLayout.NORTH );

        /*
            Initialize the YaxisTree properties as well its display size which
            indirectly determines the size of CanvasTime
        */ 
            y_tree.init();
    }

    public void init()
    {
        // time_scrollbar.init();

        // Initialize toolbar after creation of YaxisTree view
        toolbar.init();

        if ( Debug.isActive() ) {
            Debug.println( "TimelinePanel.init(): time_model = "
                         + time_model );
            Debug.println( "TimelinePanel.init(): time_scrollbar = "
                         + time_scrollbar );
            Debug.println( "TimelinePanel.init(): time_ruler = "
                         + time_ruler );
        }
    }

}
