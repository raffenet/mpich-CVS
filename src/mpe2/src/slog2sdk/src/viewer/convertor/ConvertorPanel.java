/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.convertor;

import java.awt.Color;
import java.awt.Insets;
import java.awt.Dimension;
import java.awt.Component;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JTextField;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JProgressBar;
import javax.swing.ImageIcon;
import javax.swing.AbstractButton;
import javax.swing.BorderFactory;
import javax.swing.border.Border;
import javax.swing.border.TitledBorder;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import java.util.Properties;
import java.util.StringTokenizer;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.net.URL;

import logformat.slog2.input.InputLog;
import viewer.common.Const;
import viewer.common.Dialogs;
import viewer.common.Routines;
import viewer.common.CustomCursor;
import viewer.common.ActableTextField;
import viewer.common.LogFileChooser;

public class ConvertorPanel extends JPanel
                            implements WaitingContainer
{
    private JComboBox          cmd_pulldown;
    private ActableTextField   cmd_infile;
    private JButton            infile_btn;
    private JTextField         cmd_outfile;
    private JButton            outfile_btn;
    private AdvancingTextArea  cmd_textarea;
    private JTextField         cmd_outfile_size;
    private JProgressBar       cmd_progress;
    private JTextField         cmd_option4jvm;
    private JTextField         cmd_option4jar;
    private JTextField         cmd_path2jvm;
    private JTextField         cmd_path2jardir;
    private JTextField         cmd_path2tracedir;
    private JSplitPane         cmd_splitter;
    private JButton            cmd_start_btn;
    private JButton            cmd_stop_btn;
    private JButton            cmd_close_btn;
    private JButton            cmd_help_btn;

    private Window             top_window;
    private LogFileChooser     file_chooser;
    private String             file_sep, path_sep;
    private String             err_msg;

    private SwingProcessWorker logconv_worker;

    public ConvertorPanel()
    {
        super();
        this.initComponents();
        this.initAllTextFields();

        file_chooser = new LogFileChooser( false );
        cmd_pulldown.addActionListener( new PulldownListener() );
        cmd_infile.addActionListener( new LogNameListener() );
        infile_btn.addActionListener( new InputFileSelectorListener() );
        outfile_btn.addActionListener( new OutputFileSelectorListener() );
        cmd_start_btn.addActionListener( new StartConvertorListener() );
        cmd_stop_btn.addActionListener( new StopConvertorListener() );
        cmd_help_btn.addActionListener( new HelpConvertorListener() );
        this.finalizeWaiting();

        logconv_worker = null;
    }

    public void init( String filename )
    {
        top_window = SwingUtilities.windowForComponent( this );
        cmd_splitter.setDividerLocation( 1.0d );
        if ( filename != null && filename.length() > 0 ) {
            cmd_infile.setText( filename );
            cmd_infile.fireActionPerformed();
            cmd_pulldown.setSelectedItem(
                         ConvertorConst.getDefaultConvertor( filename ) );
        }
        if ( err_msg != null )
            Dialogs.error( top_window, err_msg );
    }



    private URL getURL( String filename )
    {
        URL url = null;
        url = getClass().getResource( filename );
        return url;
    }

    private void initComponents()
    {
        Border   lowered_border, etched_border;
        lowered_border  = BorderFactory.createLoweredBevelBorder();
        etched_border   = BorderFactory.createEtchedBorder();

        //  Setup all relevant Dimension of various components
        Dimension   row_pref_sz;   // for typical row JPanel
        Dimension   lbl_pref_sz;   // for all JLabel
        Dimension   fld_pref_sz;   // for all JTextField
        Dimension   pfld_pref_sz;  // for JProgressBar
        Dimension   pbar_pref_sz;  // for JTextField of Output File Size
        row_pref_sz  = new Dimension( 410, 30 );
        lbl_pref_sz  = new Dimension( 130, 26 );
        fld_pref_sz  = new Dimension( row_pref_sz.width - lbl_pref_sz.width,
                                      lbl_pref_sz.height );
        pfld_pref_sz = new Dimension( lbl_pref_sz.width,
                                      2 * lbl_pref_sz.height );
        pbar_pref_sz = new Dimension( row_pref_sz.width,
                                      pfld_pref_sz.height );

        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

            Color thumb_color, pulldown_bg_color;
            thumb_color = UIManager.getColor( "ScrollBar.thumb" );
            pulldown_bg_color = Routines.getSlightBrighterColor( thumb_color );

            JLabel  label;
            Insets  btn_insets;
            URL     icon_URL;

            JPanel  upper_panel = new JPanel();
            upper_panel.setAlignmentX( Component.CENTER_ALIGNMENT );
            upper_panel.setLayout( new BoxLayout( upper_panel,
                                                  BoxLayout.Y_AXIS ) );
            upper_panel.add( Box.createVerticalStrut( 4 ) );

                JPanel  cmd_name_panel = new JPanel();
                cmd_name_panel.setAlignmentX( Component.CENTER_ALIGNMENT );
                cmd_name_panel.setLayout( new BoxLayout( cmd_name_panel,
                                                         BoxLayout.X_AXIS ) );
                cmd_name_panel.add( Box.createHorizontalStrut( 5 ) );

                    cmd_pulldown = new JComboBox();
                    cmd_pulldown.setForeground( Color.yellow );
                    cmd_pulldown.setBackground( pulldown_bg_color );
                    cmd_pulldown.setToolTipText( " Logfile Convertor's Name " );
                    cmd_pulldown.addItem( ConvertorConst.CLOG_TO_SLOG2 );
                    cmd_pulldown.addItem( ConvertorConst.RLOG_TO_SLOG2 );
                    cmd_pulldown.addItem( ConvertorConst.UTE_TO_SLOG2 );
                    cmd_pulldown.setBorder( lowered_border );
                    cmd_pulldown.setEditable( false );
                    cmd_pulldown.setAlignmentX( Component.CENTER_ALIGNMENT );
                cmd_name_panel.add( cmd_pulldown );
                cmd_name_panel.add( Box.createHorizontalStrut( 5 ) );
                Routines.setShortJComponentSizes( cmd_name_panel,
                                                  row_pref_sz );

            upper_panel.add( cmd_name_panel );
            upper_panel.add( Box.createVerticalStrut( 4 ) );

                btn_insets      = new Insets( 1, 1, 1, 1 );

                JPanel  cmd_infile_panel = new JPanel();
                cmd_infile_panel.setAlignmentX( Component.CENTER_ALIGNMENT );
                cmd_infile_panel.setLayout( new BoxLayout( cmd_infile_panel,
                                                           BoxLayout.X_AXIS ) );

                    label = new JLabel( " Input File Spec. : " );
                    Routines.setShortJComponentSizes( label, lbl_pref_sz );
                cmd_infile_panel.add( label );
                    cmd_infile = new ActableTextField();
                cmd_infile_panel.add( cmd_infile );
                    icon_URL = getURL( Const.IMG_PATH + "Open24.gif" );
                    infile_btn = null;
                    if ( icon_URL != null )
                        infile_btn = new JButton( new ImageIcon( icon_URL ) );
                    else
                        infile_btn = new JButton( "Browse" );
                    infile_btn.setToolTipText( "Select a new Input Logfile" );
                    infile_btn.setMargin( btn_insets );
                cmd_infile_panel.add( infile_btn );
                Routines.setShortJComponentSizes( cmd_infile_panel,
                                                  row_pref_sz );

            upper_panel.add( cmd_infile_panel );
            upper_panel.add( Box.createVerticalStrut( 4 ) );

                JPanel  cmd_outfile_panel = new JPanel();
                cmd_outfile_panel.setAlignmentX( Component.CENTER_ALIGNMENT );
                cmd_outfile_panel.setLayout( new BoxLayout( cmd_outfile_panel,
                                                           BoxLayout.X_AXIS ) );

                    label = new JLabel( " Output File Name : " );
                    Routines.setShortJComponentSizes( label, lbl_pref_sz );
                cmd_outfile_panel.add( label );
                    cmd_outfile = new JTextField();
                cmd_outfile_panel.add( cmd_outfile );
                    icon_URL = getURL( Const.IMG_PATH + "Open24.gif" );
                    outfile_btn = null;
                    if ( icon_URL != null )
                        outfile_btn = new JButton( new ImageIcon( icon_URL ) );
                    else
                        outfile_btn = new JButton( "Browse" );
                    outfile_btn.setToolTipText( "Select a new Output Logfile" );
                    outfile_btn.setMargin( btn_insets );
                    // outfile_btn.addActionListener();
                cmd_outfile_panel.add( outfile_btn );
                Routines.setShortJComponentSizes( cmd_outfile_panel,
                                                  row_pref_sz );

            upper_panel.add( cmd_outfile_panel );
            upper_panel.add( Box.createVerticalStrut( 4 ) );

                    cmd_textarea = new AdvancingTextArea();
                    cmd_textarea.setColumns( 50 );
                    cmd_textarea.setRows( 5 );
                    cmd_textarea.setEditable( false );
                    cmd_textarea.setLineWrap( false );
                JScrollPane scroller = new JScrollPane( cmd_textarea );
                scroller.setAlignmentX( Component.CENTER_ALIGNMENT );

            upper_panel.add( scroller );
            upper_panel.add( Box.createVerticalStrut( 4 ) );

                JPanel cmd_outfile_status_panel = new JPanel();
                cmd_outfile_status_panel.setAlignmentX(
                                         Component.CENTER_ALIGNMENT );
                cmd_outfile_status_panel.setLayout(
                                   new BoxLayout( cmd_outfile_status_panel,
                                                  BoxLayout.X_AXIS ) );

                    JPanel cmd_outfile_size_panel = new JPanel();
                    cmd_outfile_size_panel.setAlignmentY(
                                       Component.CENTER_ALIGNMENT );
                    cmd_outfile_size_panel.setLayout(
                                     new BoxLayout( cmd_outfile_size_panel,
                                                    BoxLayout.X_AXIS ) );
                    cmd_outfile_size_panel.setBorder(
                        new TitledBorder( etched_border,
                                          " Output File Size ") );
                        cmd_outfile_size = new JTextField();
                        cmd_outfile_size.setEditable( false );
                    cmd_outfile_size_panel.add( cmd_outfile_size );
                    Routines.setShortJComponentSizes( cmd_outfile_size_panel,
                                                      pfld_pref_sz );

                cmd_outfile_status_panel.add( cmd_outfile_size_panel );

                    JPanel cmd_progress_panel = new JPanel();
                    cmd_progress_panel.setAlignmentY(
                                       Component.CENTER_ALIGNMENT );
                    cmd_progress_panel.setLayout(
                                       new BoxLayout( cmd_progress_panel,
                                                      BoxLayout.X_AXIS ) );
                    cmd_progress_panel.setBorder(
                        new TitledBorder( etched_border,
                            " Output to Input Logfile Size Ratio " ) );
                        cmd_progress = new JProgressBar();
                        cmd_progress.setStringPainted( true );
                    cmd_progress_panel.add( cmd_progress );
                    Routines.setShortJComponentSizes( cmd_progress_panel,
                                                      pbar_pref_sz );

                cmd_outfile_status_panel.add( cmd_progress_panel );

            upper_panel.add( cmd_outfile_status_panel );



        row_pref_sz  = new Dimension( 410, 27 );
        lbl_pref_sz  = new Dimension( 130, 25 );
        fld_pref_sz  = new Dimension( row_pref_sz.width - lbl_pref_sz.width,
                                      lbl_pref_sz.height );

            ActionListener  help_msg_listener;
            help_msg_listener = new ActionListener() {
                public void actionPerformed( ActionEvent evt ) {
                     printSelectedConvertorHelp();
                }
            };

            JPanel  lower_panel = new JPanel();
            lower_panel.setAlignmentX( Component.CENTER_ALIGNMENT );
            lower_panel.setLayout( new BoxLayout( lower_panel,
                                                  BoxLayout.Y_AXIS ) );
            lower_panel.add( Box.createVerticalStrut( 4 ) );

                JPanel  cmd_path2jvm_panel = new JPanel();
                cmd_path2jvm_panel.setAlignmentX(
                                   Component.CENTER_ALIGNMENT );
                cmd_path2jvm_panel.setLayout(
                                   new BoxLayout( cmd_path2jvm_panel,
                                                  BoxLayout.X_AXIS ) );

                    label = new JLabel( " JVM pathname : " );
                    Routines.setShortJComponentSizes( label, lbl_pref_sz );
                cmd_path2jvm_panel.add( label );
                    cmd_path2jvm = new JTextField();
                    // Routines.setShortJComponentSizes( cmd_path2jvm,
                    //                                   fld_pref_sz );
                    cmd_path2jvm.addActionListener( help_msg_listener );
                cmd_path2jvm_panel.add( cmd_path2jvm );
                Routines.setShortJComponentSizes( cmd_path2jvm_panel,
                                                  row_pref_sz );

            lower_panel.add( cmd_path2jvm_panel );
            lower_panel.add( Box.createVerticalGlue() );
            lower_panel.add( Box.createVerticalStrut( 4 ) );

                JPanel  cmd_option4jvm_panel = new JPanel();
                cmd_option4jvm_panel.setAlignmentX(
                                     Component.CENTER_ALIGNMENT );
                cmd_option4jvm_panel.setLayout(
                                     new BoxLayout( cmd_option4jvm_panel,
                                                    BoxLayout.X_AXIS ) );
    
                    label = new JLabel( " JVM option : " );
                    Routines.setShortJComponentSizes( label, lbl_pref_sz );
                cmd_option4jvm_panel.add( label );
                    cmd_option4jvm = new JTextField();
                    // Routines.setShortJComponentSizes( cmd_option4jvm,
                    //                                   fld_pref_sz );
                    cmd_option4jvm.addActionListener( help_msg_listener );
                cmd_option4jvm_panel.add( cmd_option4jvm );
                Routines.setShortJComponentSizes( cmd_option4jvm_panel,
                                                  row_pref_sz );
    
            lower_panel.add( cmd_option4jvm_panel );
            lower_panel.add( Box.createVerticalGlue() );
            lower_panel.add( Box.createVerticalStrut( 4 ) );
    
                JPanel  cmd_path2jardir_panel = new JPanel();
                cmd_path2jardir_panel.setAlignmentX(
                                      Component.CENTER_ALIGNMENT );
                cmd_path2jardir_panel.setLayout(
                                      new BoxLayout( cmd_path2jardir_panel,
                                                     BoxLayout.X_AXIS ) );
    
                    label = new JLabel( " JAR directory : " );
                    Routines.setShortJComponentSizes( label, lbl_pref_sz );
                cmd_path2jardir_panel.add( label );
                    cmd_path2jardir = new JTextField();
                    // Routines.setShortJComponentSizes( cmd_path2jardir,
                    //                                   fld_pref_sz );
                    cmd_path2jardir.addActionListener( help_msg_listener );
                cmd_path2jardir_panel.add( cmd_path2jardir );
                Routines.setShortJComponentSizes( cmd_path2jardir_panel,
                                                  row_pref_sz );
    
            lower_panel.add( cmd_path2jardir_panel );
            lower_panel.add( Box.createVerticalGlue() );
            lower_panel.add( Box.createVerticalStrut( 4 ) );

                JPanel  cmd_option4jar_panel = new JPanel();
                cmd_option4jar_panel.setAlignmentX(
                                     Component.CENTER_ALIGNMENT );
                cmd_option4jar_panel.setLayout(
                                     new BoxLayout( cmd_option4jar_panel,
                                                    BoxLayout.X_AXIS ) );

                    label = new JLabel( " JAR option : " );
                    Routines.setShortJComponentSizes( label, lbl_pref_sz );
                cmd_option4jar_panel.add( label );
                    cmd_option4jar = new JTextField();
                    // Routines.setShortJComponentSizes( cmd_option4jar,
                    //                                   fld_pref_sz );
                    cmd_option4jar.addActionListener( help_msg_listener );
                cmd_option4jar_panel.add( cmd_option4jar );
                Routines.setShortJComponentSizes( cmd_option4jar_panel,
                                                  row_pref_sz );

            lower_panel.add( cmd_option4jar_panel );
            lower_panel.add( Box.createVerticalGlue() );
            lower_panel.add( Box.createVerticalStrut( 4 ) );
    
                JPanel  cmd_path2tracedir_panel = new JPanel();
                cmd_path2tracedir_panel.setAlignmentX(
                                        Component.CENTER_ALIGNMENT );
                cmd_path2tracedir_panel.setLayout(
                                        new BoxLayout( cmd_path2tracedir_panel,
                                                       BoxLayout.X_AXIS ) );
    
                    label = new JLabel( " Trace directory : " );
                    Routines.setShortJComponentSizes( label, lbl_pref_sz );
                cmd_path2tracedir_panel.add( label );
                    cmd_path2tracedir = new JTextField();
                    // Routines.setShortJComponentSizes( cmd_path2tracedir,
                    //                                   fld_pref_sz );
                    cmd_path2tracedir.addActionListener( help_msg_listener );
                cmd_path2tracedir_panel.add( cmd_path2tracedir );
                Routines.setShortJComponentSizes( cmd_path2tracedir_panel,
                                                  row_pref_sz );

            lower_panel.add( cmd_path2tracedir_panel );
            lower_panel.add( Box.createVerticalStrut( 4 ) );


            cmd_splitter = new JSplitPane( JSplitPane.VERTICAL_SPLIT, true,
                                           upper_panel, lower_panel );
            cmd_splitter.setAlignmentX( Component.CENTER_ALIGNMENT );
            cmd_splitter.setOneTouchExpandable( true );
            err_msg = null;
            try {
                cmd_splitter.setResizeWeight( 1.0d );
            } catch ( NoSuchMethodError err ) {
                err_msg =
                  "Method JSplitPane.setResizeWeight() cannot be found.\n"
                + "This indicates you are running an older Java2 RunTime,\n"
                + "like the one in J2SDK 1.2.2 or older. If this is the case,\n"
                + "some features in Convertor window may not work correctly,\n"
                + "For instance, resize of the window may not resize upper \n"
                + "TextArea.  Manuel movement of splitter is needed.\n";
            }

       super.add( cmd_splitter );
       super.add( Box.createVerticalStrut( 4 ) );

            JPanel  cmd_button_panel = new JPanel();
            cmd_button_panel.setLayout( new BoxLayout( cmd_button_panel,
                                                       BoxLayout.X_AXIS ) );
            cmd_button_panel.setAlignmentX( Component.CENTER_ALIGNMENT );
            cmd_button_panel.add( Box.createHorizontalGlue() );

                btn_insets          = new Insets( 2, 4, 2, 4 );

                cmd_start_btn = new JButton( "Convert" );
                icon_URL = getURL( Const.IMG_PATH + "Copy24.gif" );
                if ( icon_URL != null ) {
                    cmd_start_btn.setIcon( new ImageIcon( icon_URL ) );
                    cmd_start_btn.setVerticalTextPosition(
                                  AbstractButton.CENTER );
                    cmd_start_btn.setHorizontalTextPosition(
                                  AbstractButton.RIGHT );
                    cmd_start_btn.setMargin( btn_insets );
                }
                cmd_start_btn.setToolTipText(
                    "Proceed with the selected logfile conversion." );
            cmd_button_panel.add( cmd_start_btn );
            cmd_button_panel.add( Box.createHorizontalGlue() );

                cmd_stop_btn = new JButton( " Stop " );
                icon_URL = getURL( Const.IMG_PATH + "Stop24.gif" );
                if ( icon_URL != null ) {
                    cmd_stop_btn.setIcon( new ImageIcon( icon_URL ) );
                    cmd_stop_btn.setVerticalTextPosition(
                                 AbstractButton.CENTER );
                    cmd_stop_btn.setHorizontalTextPosition(
                                 AbstractButton.RIGHT );
                    // cmd_stop_btn.setMargin( btn_insets );
                }
                cmd_stop_btn.setToolTipText(
                    "Stop the ongoing logfile conversion." );
            cmd_button_panel.add( cmd_stop_btn );
            cmd_button_panel.add( Box.createHorizontalGlue() );

                cmd_close_btn = new JButton( "Return" );
                icon_URL = getURL( Const.IMG_PATH + "Home24.gif" );
                if ( icon_URL != null ) {
                    cmd_close_btn.setIcon( new ImageIcon( icon_URL ) );
                    cmd_close_btn.setVerticalTextPosition(
                                  AbstractButton.CENTER );
                    cmd_close_btn.setHorizontalTextPosition(
                                  AbstractButton.RIGHT );
                    // cmd_close_btn.setMargin( btn_insets );
                }
                cmd_close_btn.setToolTipText(
                    "Close this panel and Return to the previous component." );
            cmd_button_panel.add( cmd_close_btn );
            cmd_button_panel.add( Box.createHorizontalGlue() );

                cmd_help_btn = new JButton( " Help " );
                icon_URL = getURL( Const.IMG_PATH + "Help24.gif" );
                if ( icon_URL != null ) {
                    cmd_help_btn.setIcon( new ImageIcon( icon_URL ) );
                    cmd_help_btn.setVerticalTextPosition(
                                 AbstractButton.CENTER );
                    cmd_help_btn.setHorizontalTextPosition(
                                 AbstractButton.RIGHT );
                    // cmd_help_btn.setMargin( btn_insets );
                }
                cmd_help_btn.setToolTipText(
                    "Help message of the selected logfile conversion." );
            cmd_button_panel.add( cmd_help_btn );

            cmd_button_panel.add( Box.createHorizontalGlue() );

        super.add( cmd_button_panel );
    }

    private void initAllTextFields()
    {
        Properties       sys_pptys;
        String           java_home, path2jvm;
        String           classpath, path2jardir;
        String           option4jvm;
        File             jvm_file;
        StringTokenizer  paths;
        String           path;
        char             old_char, new_char;
        int              char_idx;

        sys_pptys  = System.getProperties();
        file_sep   = sys_pptys.getProperty( "file.separator" );
        path_sep   = sys_pptys.getProperty( "path.separator" );

        // set the path to JVM 
        java_home  = sys_pptys.getProperty( "java.home" );
        path2jvm   = java_home + file_sep + "bin" + file_sep + "java";
        jvm_file   = new File( path2jvm );
        if ( ! jvm_file.exists() )
            path2jvm  = "java";
        cmd_path2jvm.setText( path2jvm );

        // set the path to all the jar files
        classpath    = sys_pptys.getProperty( "java.class.path" );
        // System.out.println( "java.class.path = " + classpath );
        path2jardir  = null;
        paths        = new StringTokenizer( classpath, path_sep );
        while ( paths.hasMoreTokens() && path2jardir == null ) {
            path      = paths.nextToken();
            char_idx  = path.lastIndexOf( file_sep );
            if ( char_idx >= 0 )
                path2jardir = path.substring( 0, char_idx );
        }
        // System.out.println( "path2jardir = " + path2jardir );
        cmd_path2jardir.setText( path2jardir );

        // set the JVM option
        option4jvm  = null;
        try {
            option4jvm  = cmd_option4jvm.getText();
        } catch ( NullPointerException err ) {}
        if ( option4jvm == null || option4jvm.length() <= 0 );
            cmd_option4jvm.setText( "-Xms32m -Xmx64m" );

        // Replace file separator if necessary
        ConvertorConst.updateFileSeparatorOfTraceDirs( file_sep.charAt( 0 ) );
    }



    public String selectLogFile()
    {
        int   istat;
        istat = file_chooser.showOpenDialog( top_window );
        if ( istat == LogFileChooser.APPROVE_OPTION ) {
            File   selected_file, selected_dir;
            selected_file = file_chooser.getSelectedFile();
            if ( selected_file != null ) {
                selected_dir  = selected_file.getParentFile();
                if ( selected_dir != null )
                    file_chooser.setCurrentDirectory( selected_dir );
                return selected_file.getPath();
            }
        }
        else
            Dialogs.info( top_window, "No file chosen", null );
        return null;
    }

    private void printSelectedConvertorHelp()
    {
        String             convertor;
        String             path2jardir, path2tracedir;
        String             jar_path;
        StringBuffer       exec_cmd;
        File               jar_file;
        Runtime            runtime;
        Process            proc;
        InputStreamThread  proc_err_task, proc_out_task;

        convertor = (String) cmd_pulldown.getSelectedItem();

        //  Set the path to the jar file
        path2jardir = cmd_path2jardir.getText();
        if ( path2jardir != null && path2jardir.length() > 0 )
            jar_path  = path2jardir + file_sep
                      + ConvertorConst.getDefaultJarName( convertor );
        else
            jar_path  = ConvertorConst.getDefaultJarName( convertor );
        jar_file  = new File( jar_path );
        if ( ! jar_file.exists() ) {
            Dialogs.error( top_window, jar_path + " does not exist!" );
            return;
        }

        exec_cmd = new StringBuffer( cmd_path2jvm.getText() + " "
                                   + cmd_option4jvm.getText() );
        path2tracedir = cmd_path2tracedir.getText();
        if ( path2tracedir != null && path2tracedir.length() > 0 ) {
            exec_cmd.append( " -Djava.library.path=" );
            if (    path2tracedir.startsWith( file_sep )
                 || path2tracedir.charAt( 1 ) == ':' )
                // Assume it is full path
                exec_cmd.append( path2tracedir );
            else
                // Assume it is relative path to JAR libdir
                exec_cmd.append( path2jardir + file_sep
                               + path2tracedir );
        }
        exec_cmd.append( " -jar " + jar_path + " -h" );

        cmd_textarea.append( "Executing " + exec_cmd.toString()
                           + " ...." );
        runtime  = Runtime.getRuntime();
        try {
            proc = runtime.exec( exec_cmd.toString() );
            proc_err_task = new InputStreamThread( proc.getErrorStream(),
                                                   "Error", cmd_textarea );
            proc_out_task = new InputStreamThread( proc.getInputStream(),
                                                   "Output", cmd_textarea );
            proc_err_task.start();
            proc_out_task.start();

            // Block THIS thread till process returns!
            int proc_istatus = proc.waitFor();
            // Clean up InputStreamThread's when the proces is done.
            proc_err_task.stopRunning();
            proc_err_task = null;
            proc_out_task.stopRunning();
            proc_out_task = null;

            cmd_textarea.append( "\n> Ending with exit status "
                               + proc_istatus + "\n" );
        } catch ( Throwable err ) {
            err.printStackTrace();
        }
    }


    private void convertSelectedLogfile()
    {
        String             convertor;
        String             path2jardir, path2tracedir;
        String             infile_name, outfile_name, jar_path;
        String             option4jar;
        File               infile, outfile, jar_file;
        InputLog           slog_ins;
        StringBuffer       exec_cmd;
        ProgressAction     logconv_progress;

        // Check the validity of the Input File
        infile_name   = cmd_infile.getText();
        infile        = new File( infile_name );
        if ( ! infile.exists() ) {
            Dialogs.error( top_window,
                           infile_name + " does not exist!\n"
                         + "No conversion will take place." );
            return;
        }
        if ( infile.isDirectory() ) {
            Dialogs.error( top_window,
                           infile_name + " is a directory!\n"
                         + "No conversion will take place." );
            return;
        }
        if ( ! infile.canRead() ) {
            Dialogs.error( top_window,
                           "File " + infile_name + " cannot be read!\n"
                         + "No conversion will take place." );
            return;
        }
        slog_ins = null;
        try {
            slog_ins = new InputLog( infile_name );
        } catch ( NullPointerException nperr ) {
            slog_ins = null;
        } catch ( Exception err ) {
            slog_ins = null;
        }
        if ( slog_ins != null && slog_ins.isSLOG2() ) {
            Dialogs.error( top_window,
                           infile_name + " is already a SLOG-2 file!\n"
                         + "No conversion will take place." );
            cmd_outfile.setText( infile_name );
            return;
        }

        // Check the validity of the Output File
        outfile_name  = cmd_outfile.getText();
        outfile       = new File( outfile_name );
        if ( outfile.exists() ) {
            if ( outfile.isDirectory() ) {
                Dialogs.error( top_window,
                               outfile_name + " is a directory!\n"
                             + "No conversion will take place." );
                return;
            }
            if ( ! outfile.canWrite() ) {
                Dialogs.error( top_window,
                               "File " + outfile_name + " cannot be written!\n"
                             + "No conversion will take place." );
                return;
            }
            if ( ! Dialogs.confirm( top_window,
                                    outfile_name + " already exists! "
                                  + "Do you want to overwrite it ?" ) ) {
                Dialogs.info( top_window,
                              "Please change the output filename "
                            + "and restart the conversion again.",
                              null );
                return;
            }
            outfile.delete();
        }

        convertor = (String) cmd_pulldown.getSelectedItem();

        //  Set the path to the jar file
        path2jardir = cmd_path2jardir.getText();
        if ( path2jardir != null && path2jardir.length() > 0 )
            jar_path  = path2jardir + file_sep
                      + ConvertorConst.getDefaultJarName( convertor );
        else
            jar_path  = ConvertorConst.getDefaultJarName( convertor );
        jar_file  = new File( jar_path );
        if ( ! jar_file.exists() ) {
            Dialogs.error( top_window, jar_path + " does not exist!" );
            return;
        }

        exec_cmd = new StringBuffer( cmd_path2jvm.getText() + " "
                                   + cmd_option4jvm.getText() );
        path2tracedir = cmd_path2tracedir.getText();
        if ( path2tracedir != null && path2tracedir.length() > 0 ) {
            exec_cmd.append( " -Djava.library.path=" );
            if (    path2tracedir.startsWith( file_sep )
                 || path2tracedir.charAt( 1 ) == ':' )
                // Assume it is full path
                exec_cmd.append( path2tracedir );
            else
                // Assume it is relative path to JAR libdir
                exec_cmd.append( path2jardir + file_sep
                               + path2tracedir );
        }
        exec_cmd.append( " -jar " + jar_path );
        
        option4jar  = cmd_option4jar.getText();
        if ( option4jar != null && option4jar.length() > 0 )
            exec_cmd.append( " " + option4jar );

        exec_cmd.append( " -o " + outfile_name + " " + infile_name );

        /*
           Start a SwingWorker thread to execute the process:
           Prepare a progress action for the JProgressBar for the SwingWorker
        */
        logconv_progress = new ProgressAction( cmd_outfile_size, cmd_progress );
        logconv_progress.initialize( infile, outfile );
        logconv_worker = new SwingProcessWorker( this, cmd_textarea );
        logconv_worker.initialize( exec_cmd.toString(), logconv_progress );
        logconv_worker.start();
    }

    private void resetAllButtons( boolean isConvertingLogfile )
    {
        cmd_start_btn.setEnabled( !isConvertingLogfile );
        cmd_stop_btn.setEnabled( isConvertingLogfile );
        cmd_close_btn.setEnabled( !isConvertingLogfile );
        cmd_help_btn.setEnabled( !isConvertingLogfile );
    }

    // Interface for WaitingContainer (used by SwingProcessWorker)
    public void initializeWaiting()
    {
        Routines.setComponentAndChildrenCursors( cmd_splitter,
                                                 CustomCursor.Wait );
        this.resetAllButtons( true );
    }

    // Interface for WaitingContainer (used by SwingProcessWorker)
    public void finalizeWaiting()
    {
        this.resetAllButtons( false );
        Routines.setComponentAndChildrenCursors( cmd_splitter,
                                                 CustomCursor.Normal );
    }

    public void addActionListenerForCloseButton( ActionListener action )
    {
        if ( action != null )
            cmd_close_btn.addActionListener( action );
    }

    public String getOutputSLOG2Name()
    {
        return cmd_outfile.getText();
    }



    private class LogNameListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            String infile_name, outfile_name;
            infile_name   = cmd_infile.getText();
            outfile_name  = ConvertorConst.getDefaultSLOG2Name( infile_name );
            cmd_outfile.setText( outfile_name );
        }
    }

    private class InputFileSelectorListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            String filename = selectLogFile();
            if ( filename != null && filename.length() > 0 ) {
                cmd_infile.setText( filename );
                printSelectedConvertorHelp();
            }
        }
    }

    private class OutputFileSelectorListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            String filename = selectLogFile();
            if ( filename != null && filename.length() > 0 ) {
                cmd_outfile.setText( filename );
            }
        }
    }

    private class PulldownListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            String  convertor;
            convertor  = (String) cmd_pulldown.getSelectedItem();
            cmd_path2tracedir.setText(
                              ConvertorConst.getDefaultTraceDir( convertor ) );
            printSelectedConvertorHelp();
        }
    }

    private class StartConvertorListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            convertSelectedLogfile();
        }
    }

    private class StopConvertorListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            if ( logconv_worker != null )
                logconv_worker.finished();
        }
    }

    private class HelpConvertorListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            printSelectedConvertorHelp();
        }
    }
}
