/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.first;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.Border;
import java.net.URL;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.EOFException;
import java.io.IOException;
import java.util.List;
import java.util.Iterator;

import logformat.slog2.LineIDMap;
import logformat.slog2.input.InputLog;
import viewer.common.Dialogs;
import viewer.common.TopWindow;
import viewer.common.Parameters;
import viewer.common.PreferenceFrame;
import viewer.legends.LegendFrame;
import viewer.timelines.TimelineFrame;

public class FirstPanel extends JPanel
                                implements ActionListener, ItemListener
{
    private static String       open_icon_path   = "/images/Open24.gif";
    private static String       show_icon_path   = "/images/New24.gif";
    private static String       close_icon_path  = "/images/Stop24.gif";
    private static String       legend_icon_path = "/images/Properties24.gif";
    private static String       prefer_icon_path = "/images/Preferences24.gif";

    private        JTextField   logname_fld;
    private        JButton      file_open_btn;
    private        JComboBox    pulldown_list;
    private        JButton      file_show_btn;

    /*  hidden buttons */
    private        JButton      file_close_btn;
    private        JButton      edit_legend_btn;
    private        JButton      edit_prefer_btn;

    private        InputLog         slog_ins;
    private        int              view_ID;
    private        PreferenceFrame  pptys_frame;
    private        LegendFrame      legend_frame;
    private        TimelineFrame    timeline_frame;

    public FirstPanel( String filename, int view_idx )
    {
        super();
        this.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        slog_ins = null;
        view_ID  = view_idx;

        Border   raised_border, lowered_border, etched_border;
        raised_border   = BorderFactory.createRaisedBevelBorder();
        lowered_border  = BorderFactory.createLoweredBevelBorder();
        etched_border   = BorderFactory.createEtchedBorder();

        JLabel  label;
        URL     icon_URL;
            JPanel logname_panel = new JPanel();
            logname_panel.setLayout( new BoxLayout( logname_panel,
                                                    BoxLayout.X_AXIS ) );
            logname_panel.setAlignmentX( Component.CENTER_ALIGNMENT );

                label = new JLabel( " LogName : " );
            logname_panel.add( label );
                logname_fld = new JTextField( filename, 40 );
                logname_fld.setBorder( BorderFactory.createCompoundBorder(
                                       lowered_border, etched_border ) );
                logname_fld.addActionListener( this );
            logname_panel.add( logname_fld );
            logname_panel.add( Box.createHorizontalStrut( 40 ) );
                icon_URL = null;
                icon_URL = getURL( open_icon_path );
                if ( icon_URL != null )
                    file_open_btn = new JButton( new ImageIcon( icon_URL ) );
                else
                    file_open_btn = new JButton( "OPEN" );
                file_open_btn.setToolTipText( "Open/Initialize the logfile" );
                file_open_btn.setBorder( raised_border );
                file_open_btn.addActionListener( this );
            logname_panel.add( file_open_btn );
                /* file_close_btn is a hidden button */
                icon_URL = null;
                icon_URL = getURL( close_icon_path );
                if ( icon_URL != null )
                    file_close_btn = new JButton( new ImageIcon( icon_URL ) );
                else
                    file_close_btn = new JButton( "CLOSE" );
                file_close_btn.setToolTipText( "Close the logfile" );
                file_close_btn.setBorder( raised_border );
                file_close_btn.addActionListener( this );
                /* edit_legend_btn is a hidden button */
                icon_URL = null;
                icon_URL = getURL( legend_icon_path );
                if ( icon_URL != null )
                    edit_legend_btn = new JButton( new ImageIcon( icon_URL ) );
                else
                    edit_legend_btn = new JButton( "LEGEND" );
                edit_legend_btn.setToolTipText( "Open Legend window" );
                edit_legend_btn.setBorder( raised_border );
                edit_legend_btn.addActionListener( this );
                /* edit_prefer_btn is a hidden button */
                icon_URL = null;
                icon_URL = getURL( prefer_icon_path );
                if ( icon_URL != null )
                    edit_prefer_btn = new JButton( new ImageIcon( icon_URL ) );
                else
                    edit_prefer_btn = new JButton( "PREFERENCE" );
                edit_prefer_btn.setToolTipText( "Open Preference window" );
                edit_prefer_btn.setBorder( raised_border );
                edit_prefer_btn.addActionListener( this );

        super.add( logname_panel );
        super.add( Box.createVerticalStrut( 4 ) );

            JPanel map_panel = new JPanel();
            map_panel.setLayout( new BoxLayout( map_panel,
                                                BoxLayout.X_AXIS ) );
            map_panel.setAlignmentX( Component.CENTER_ALIGNMENT );

                label = new JLabel( " ViewMap : " );
            map_panel.add( label );
                pulldown_list = new JComboBox();
                /*
                pulldown_list.setBorder( BorderFactory.createCompoundBorder(
                                         etched_border, lowered_border ) );
                */
                pulldown_list.setBorder( lowered_border );
                pulldown_list.addItemListener( this );
            map_panel.add( pulldown_list );
            map_panel.add( Box.createHorizontalStrut( 40 ) );
                icon_URL = null;
                icon_URL = getURL( show_icon_path );
                if ( icon_URL != null )
                    file_show_btn = new JButton( new ImageIcon( icon_URL ) );
                else
                    file_show_btn = new JButton( "SHOW" );
                file_show_btn.setToolTipText( "Show/Display the logfile" );
                file_show_btn.setBorder( raised_border );
                file_show_btn.addActionListener( this );
            map_panel.add( file_show_btn );

        super.add( map_panel );
    }

    public void init()
    {
        /*  Initialization  */
        Parameters.initSetupFile();
        Parameters.readFromSetupFile( TopWindow.First.getWindow() );
        Parameters.initStaticClasses();
        pptys_frame = new PreferenceFrame();
        pptys_frame.setVisible( false );
    }

    private URL getURL( String filename )
    {
        return getClass().getResource( filename );
    }

    public JTextField getLogNameTextField()
    {
        return logname_fld;
    }

    public JButton getLogFileCloseButton()
    {
        return file_close_btn;
    }

    public JButton getEditLegendButton()
    {
        return edit_legend_btn;
    }

    public JButton getEditPreferenceButton()
    {
        return edit_prefer_btn;
    }

    public void actionPerformed( ActionEvent evt )
    {
        Object evt_src = evt.getSource();
        if (    evt_src == this.file_open_btn 
             || evt_src == this.logname_fld ) {
            String err_msg = null;
            this.disposeInputLogResources();
            slog_ins  = this.getInputLog( logname_fld.getText() );
            if ( slog_ins != null ) {
                if ( (err_msg = slog_ins.getCompatibleHeader() ) != null ) {
                    if ( ! Dialogs.confirm( TopWindow.First.getWindow(),
                                    err_msg
                                  + logformat.slog2.Const.VERSION_HISTORY 
                                  + "Do you still want to continue reading "
                                  + "the logfile ?" ) ) {
                         slog_ins = null;
                         return;
                    }
                }
                slog_ins.initialize();
                this.setMapPullDownMenu( (List) slog_ins.getLineIDMapList() );
                legend_frame = new LegendFrame( slog_ins );
                legend_frame.pack();
                TopWindow.layoutIdealLocations();
                legend_frame.setVisible( true );
            }
            else
                Dialogs.error( TopWindow.First.getWindow(), "Null logfile!" );
        }
        else if ( evt_src == this.file_show_btn ) {
            if ( slog_ins != null && view_ID >= 0 ) {
                timeline_frame  = new TimelineFrame( slog_ins, view_ID );
                timeline_frame.pack();
                TopWindow.layoutIdealLocations();
                timeline_frame.setVisible( true );
                timeline_frame.init();
            }
        }
        else if ( evt_src == this.edit_legend_btn ) {
            if ( slog_ins != null && legend_frame != null ) {
                legend_frame.pack();
                TopWindow.layoutIdealLocations();
                legend_frame.setVisible( true );
            }
        }
        else if ( evt_src == this.edit_prefer_btn ) {
            if ( pptys_frame != null ) {
                pptys_frame.pack();
                TopWindow.layoutIdealLocations();
                pptys_frame.setVisible( true );
                pptys_frame.toFront();
            }
        }
        else if ( evt_src == this.file_close_btn )
            this.disposeInputLogResources();
        else 
            Dialogs.error( TopWindow.First.getWindow(), "Undefined Action!" );
    }

    public void itemStateChanged( ItemEvent ievt )
    {
        if ( ievt.getStateChange() == ItemEvent.SELECTED ) {
            // System.out.println( "item event = " + ievt );
            view_ID = pulldown_list.getSelectedIndex();
            if ( view_ID <= -1 ) {
                Dialogs.error( TopWindow.First.getWindow(),
                               "Invalid view_ID, reset it to 0!" );
                view_ID = 0;
            }
        }
    }

    /* This disposes all the windows and InputLog related resources. */
    private void disposeInputLogResources()
    {
        if ( slog_ins != null ) {
            TopWindow.Legend.disposeAll();
            pulldown_list.removeAllItems();
            slog_ins.close();
            slog_ins        = null;
            legend_frame    = null;
            timeline_frame  = null;
        }
    }

    private void setMapPullDownMenu( List list )
    {
        String map_title;
        pulldown_list.removeAllItems();
        Iterator linemaps = list.iterator();
        /* It is crucual to add LineIDMapList tn the order it is created */
        while ( linemaps.hasNext() ) {
            map_title = "  " + ( (LineIDMap) linemaps.next() ).getTitle();
            pulldown_list.addItem( map_title );
        }
    }

    private InputLog getInputLog( String pathname )
    {
        String logname = pathname.trim();
        if ( logname != null && logname.length() > 0 ) {
            File logfile = new File( logname );
            if ( ! logfile.exists() ) {
                Dialogs.error( TopWindow.First.getWindow(),
                               "File Not Found when initializing "
                             + logname + "." );
                return null;
            }
            if ( ! logfile.canRead() ) {
                Dialogs.error( TopWindow.First.getWindow(),
                               "File " + logname + " cannot be read." );
                return null;
            }

            InputLog slog = null;
            try {
                slog = new InputLog( logname );
            } catch ( NullPointerException nperr ) {
                Dialogs.error( TopWindow.First.getWindow(),
                               "NullPointerException when initializing "
                             + logname + "!" );
                return null;
            } catch ( Exception err ) {
                Dialogs.error( TopWindow.First.getWindow(),
                               "EOFException when initializing "
                             + logname + "!" );
                return null;
            }
            return slog;
        }
        else {
            Dialogs.error( TopWindow.First.getWindow(), "Null pathname!" );
            return null;
        }
        
    }
}
