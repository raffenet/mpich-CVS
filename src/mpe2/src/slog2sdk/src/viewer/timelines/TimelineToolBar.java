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
import java.util.*;
import java.net.URL;

public class TimelineToolBar extends JToolBar
{
    private ViewportTimeYaxis       canvas_vport;
    private JScrollBar              y_scrollbar;
    private YaxisTree               y_tree;
    private YaxisMaps               y_maps;
    private ScrollbarTime           time_scrollbar;
    private ModelTime               time_model;

    public  JButton                 mark_btn;
    public  JButton                 move_btn;
    public  JButton                 delete_btn;
    // public  JButton                 redo_btn;
    // public  JButton                 undo_btn;
    // public  JButton                 remove_btn;

    public  JButton                 up_btn;
    public  JButton                 down_btn;
    public  JButton                 expand_btn;
    public  JButton                 collapse_btn;
    public  JButton                 commit_btn;

    public  JButton                 backward_btn;
    public  JButton                 forward_btn;
    public  JButton                 zoomIn_btn;
    public  JButton                 home_btn;
    public  JButton                 zoomOut_btn;
    public  JButton                 zoomRedo_btn;
    public  JButton                 zoomUndo_btn;

    public  JButton                 jumpBack_btn;
    public  JButton                 jump_btn;
    public  JButton                 jumpFore_btn;

    public  JButton                 refresh_btn;
    public  JButton                 print_btn;
    public  JButton                 stop_btn;

    private String                  img_path = "/images/";

    public TimelineToolBar( ViewportTimeYaxis in_canvas_vport,
                            JScrollBar in_y_scrollbar,
                            YaxisTree in_y_tree, YaxisMaps in_y_maps,
                            ScrollbarTime in_t_scrollbar, ModelTime in_t_model )
    {
        super();
        canvas_vport     = in_canvas_vport;
        y_scrollbar      = in_y_scrollbar;
        y_tree           = in_y_tree;
        y_maps           = in_y_maps;
        time_scrollbar   = in_t_scrollbar;
        time_model       = in_t_model;
        this.addButtons();
    }

    public void init()
    {
        this.initButtons();
    }

    protected URL getURL( String filename )
    {
        URL url = null;

        url = getClass().getResource( filename );

        return url;
    }

    private void addButtons()
    {
        Dimension  mini_separator_size;
        URL        icon_URL;

        mini_separator_size = new Dimension( 5, 5 );

        icon_URL = getURL( img_path + "Up24.gif" );
        if ( icon_URL != null )
            up_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            up_btn = new JButton( "Up" );
        up_btn.setToolTipText( "scroll Upward half a screen" );
        // up_btn.setPreferredSize( btn_dim );
        up_btn.addActionListener( new ActionVportUp( y_scrollbar ) );
        super.add( up_btn );

        icon_URL = getURL( img_path + "Down24.gif" );
        if ( icon_URL != null )
            down_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            down_btn = new JButton( "Down" );
        down_btn.setToolTipText( "scroll Downward half a screen" );
        // down_btn.setPreferredSize( btn_dim );
        down_btn.addActionListener( new ActionVportDown( y_scrollbar ) );
        super.add( down_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "Edit24.gif" );
        if ( icon_URL != null )
            mark_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            mark_btn = new JButton( "Mark" );
        mark_btn.setToolTipText( "Mark the timelines" );
        // mark_btn.setPreferredSize( btn_dim );
        mark_btn.addActionListener(
                 new ActionTimelineMark( this, y_tree ) );
        super.add( mark_btn );

        icon_URL = getURL( img_path + "Paste24.gif" );
        if ( icon_URL != null )
            move_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            move_btn = new JButton( "Move" );
        move_btn.setToolTipText( "Move the marked timelines" );
        // move_btn.setPreferredSize( btn_dim );
        move_btn.addActionListener(
                 new ActionTimelineMove( this, y_tree ) );
        super.add( move_btn );

        icon_URL = getURL( img_path + "Delete24.gif" );
        if ( icon_URL != null )
            delete_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            delete_btn = new JButton( "Delete" );
        delete_btn.setToolTipText( "Delete the marked timelines" );
        // delete_btn.setPreferredSize( btn_dim );
        delete_btn.addActionListener(
                   new ActionTimelineDelete( this, y_tree ) );
        super.add( delete_btn );

        /*
        icon_URL = getURL( img_path + "Remove24.gif" );
        if ( icon_URL != null )
            remove_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            remove_btn = new JButton( "Remove" );
        remove_btn.setToolTipText( "Remove the timeline from the display" );
        // remove_btn.setPreferredSize( btn_dim );
        remove_btn.addActionListener(
            new action_timeline_remove( y_tree, list_view ) );
        super.add( remove_btn );
        */

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "TreeExpand24.gif" );
        if ( icon_URL != null )
            expand_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            expand_btn = new JButton( "Expand" );
        expand_btn.setToolTipText( "Expand the tree by 1 level" );
        // expand_btn.setPreferredSize( btn_dim );
        expand_btn.addActionListener(
                   new ActionYaxisTreeExpand( this, y_tree ) );
        super.add( expand_btn );

        icon_URL = getURL( img_path + "TreeCollapse24.gif" );
        if ( icon_URL != null )
            collapse_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            collapse_btn = new JButton( "Collapse" );
        collapse_btn.setToolTipText( "Collapse the tree by 1 level" );
        // collapse_btn.setPreferredSize( btn_dim );
        collapse_btn.addActionListener(
                     new ActionYaxisTreeCollapse( this, y_tree ) );
        super.add( collapse_btn );

        icon_URL = getURL( img_path + "TreeCommit24.gif" );
        if ( icon_URL != null )
            commit_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            commit_btn = new JButton( "Commit" );
        commit_btn.setToolTipText(
                   "Commit changes and Redraw the TimeLines Display" );
        // collapse_btn.setPreferredSize( btn_dim );
        commit_btn.addActionListener(
                   new ActionYaxisTreeCommit( this, canvas_vport, y_maps ) );
        super.add( commit_btn );

        super.addSeparator();
        super.addSeparator();

        icon_URL = getURL( img_path + "Backward24.gif" );
        if ( icon_URL != null )
            backward_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            backward_btn = new JButton( "Backward" );
        backward_btn.setToolTipText( "scroll Backward half a screen" );
        // backward_btn.setPreferredSize( btn_dim );
        backward_btn.addActionListener(
                     new ActionVportBackward( time_scrollbar ) );
        super.add( backward_btn );

        icon_URL = getURL( img_path + "Forward24.gif" );
        if ( icon_URL != null )
            forward_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            forward_btn = new JButton( "Forward" );
        forward_btn.setToolTipText( "scroll Forward half a screen" );
        // forward_btn.setPreferredSize( btn_dim );
        forward_btn.addActionListener(
                    new ActionVportForward( time_scrollbar ) );
        super.add( forward_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "Redo24.gif" );
        if ( icon_URL != null )
            zoomRedo_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomRedo_btn = new JButton( "ZoomRedo" );
        zoomRedo_btn.setToolTipText( "Redo the previous zoom operation" );
        // zoomRedo_btn.setPreferredSize( btn_dim );
        zoomRedo_btn.addActionListener(
                     new ActionZoomRedo( this, time_model ) );
        super.add( zoomRedo_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "ZoomIn24.gif" );
        if ( icon_URL != null )
            zoomIn_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomIn_btn = new JButton( "ZoomIn" );
        zoomIn_btn.setToolTipText( "Zoom In 1 level in time" );
        // zoomIn_btn.setPreferredSize( btn_dim );
        zoomIn_btn.addActionListener(
                   new ActionZoomIn( this, time_model ) );
        super.add( zoomIn_btn );

        icon_URL = getURL( img_path + "Home24.gif" );
        if ( icon_URL != null )
            home_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            home_btn = new JButton( "Home" );
        home_btn.setToolTipText( "Reset to the lowest resolution in time" );
        // home_btn.setPreferredSize( btn_dim );
        home_btn.addActionListener(
                 new ActionZoomHome( this, time_model ) );
        super.add( home_btn );

        icon_URL = getURL( img_path + "ZoomOut24.gif" );
        if ( icon_URL != null )
            zoomOut_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomOut_btn = new JButton( "ZoomOut" );
        zoomOut_btn.setToolTipText( "Zoom Out 1 level in time" );
        // zoomOut_btn.setPreferredSize( btn_dim );
        zoomOut_btn.addActionListener(
                    new ActionZoomOut( this, time_model ) );
        super.add( zoomOut_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "Undo24.gif" );
        if ( icon_URL != null )
            zoomUndo_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomUndo_btn = new JButton( "ZoomUndo" );
        zoomUndo_btn.setToolTipText( "Undo the previous zoom operation" );
        // zoomUndo_btn.setPreferredSize( btn_dim );
        zoomUndo_btn.addActionListener(
                     new ActionZoomUndo( this, time_model ) );
        super.add( zoomUndo_btn );

        super.addSeparator();
        super.addSeparator();

        icon_URL = getURL( img_path + "FindBack24.gif" );
        if ( icon_URL != null )
            jumpBack_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            jumpBack_btn = new JButton( "JumpBack" );
        jumpBack_btn.setToolTipText( "Jump backward in time" );
        // jumpBack_btn.setPreferredSize( btn_dim );
        // jumpBack_btn.addActionListener( new ActionJumpBack() );
        super.add( jumpBack_btn );

        icon_URL = getURL( img_path + "Find24.gif" );
        if ( icon_URL != null )
            jump_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            jump_btn = new JButton( "Jump-Scroll" );
        jump_btn.setToolTipText( "Start jump-scroll window" );
        // jump_btn.setPreferredSize( btn_dim );
        // jump_btn.addActionListener( new ActionJump() );
        super.add( jump_btn );

        icon_URL = getURL( img_path + "FindFore24.gif" );
        if ( icon_URL != null )
            jumpFore_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            jumpFore_btn = new JButton( "JumpFore" );
        jumpFore_btn.setToolTipText( "Jump foreward in time" );
        // jumpFore_btn.setPreferredSize( btn_dim );
        // jumpFore_btn.addActionListener( new ActionJumpFore() );
        super.add( jumpFore_btn );

        super.addSeparator();
        super.addSeparator();

        icon_URL = getURL( img_path + "Refresh24.gif" );
        if ( icon_URL != null )
            refresh_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            refresh_btn = new JButton( "Refresh" );
        refresh_btn.setToolTipText(
                    "Refresh the screen after Preference update" );
        // refresh_btn.setPreferredSize( btn_dim );
        refresh_btn.addActionListener(
                   new ActionPptyRefresh( y_tree, commit_btn ) );
        super.add( refresh_btn );

        icon_URL = getURL( img_path + "Print24.gif" );
        if ( icon_URL != null )
            print_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            print_btn = new JButton( "Print" );
        print_btn.setToolTipText( "Print" );
        // print_btn.setPreferredSize( btn_dim );
        print_btn.addActionListener( new ActionPptyPrint() );
        super.add( print_btn );

        icon_URL = getURL( img_path + "Stop24.gif" );
        if ( icon_URL != null )
            stop_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            stop_btn = new JButton( "Exit" );
        stop_btn.setToolTipText( "Exit" );
        // stop_btn.setPreferredSize( btn_dim );
        stop_btn.addActionListener( new ActionPptyStop() );
        super.add( stop_btn );
    }

    private void initButtons()
    {
        up_btn.setEnabled( true );
        down_btn.setEnabled( true );

        mark_btn.setEnabled( true );
        move_btn.setEnabled( false );
        delete_btn.setEnabled( false );
        // remove_btn.setEnabled( true );

        expand_btn.setEnabled( y_tree.isLevelExpandable() );
        collapse_btn.setEnabled( y_tree.isLevelCollapsable() );
        commit_btn.setEnabled( true );

        backward_btn.setEnabled( true );
        forward_btn.setEnabled( true );
        zoomIn_btn.setEnabled( true );
        home_btn.setEnabled( true );
        zoomOut_btn.setEnabled( true );

        jumpBack_btn.setEnabled( false );
        jump_btn.setEnabled( false );
        jumpFore_btn.setEnabled( false );

        refresh_btn.setEnabled( true );
        print_btn.setEnabled( true );
        stop_btn.setEnabled( true );
    }
}
