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
import javax.swing.border.EtchedBorder;
import java.io.File;
import java.net.URL;

import viewer.common.Dialogs;
import viewer.common.TopWindow;

public class FirstMenuBar extends JMenuBar
                                  implements ActionListener
{
    private static String        about_str = "Jumpshot-4, the SLOG-2 viewer\n"
                                           + "Questions: chan@mcs.anl.gov";
    private static String        manuel_pathname  = "manuel_index.html";
    private static String        faq_pathname     = "faq_index.html";
    private static String        js_icon_pathname = "/images/jumpshot.gif";

    private        boolean       isApplet;
    private        FirstPanel    first_panel;

    private        JMenuItem     file_select_item;
    private        JMenuItem     file_close_item;
    private        JMenuItem     file_exit_item;
    private        JMenuItem     edit_legends_item;
    private        JMenuItem     edit_preferences_item;
    private        JMenuItem     help_manuel_item;
    private        JMenuItem     help_faq_item;
    private        JMenuItem     help_about_item;

    public FirstMenuBar( boolean isTopApplet, FirstPanel in_panel )
    {
        super();
        super.setBorder( new EtchedBorder() );

        isApplet       = isTopApplet;
        first_panel    = in_panel;

        JMenu      menu;
            menu = new JMenu( "File" );
                file_select_item = new JMenuItem( "Select ..." );
                file_select_item.addActionListener( this );
            menu.add( file_select_item );
                file_close_item  = new JMenuItem( "Close" );
                file_close_item.addActionListener( this );
            menu.add( file_close_item );
            menu.addSeparator();
                file_exit_item   = new JMenuItem( "Exit" );
                file_exit_item.addActionListener( this );
            menu.add( file_exit_item );
        super.add( menu );
            
            menu = new JMenu( "Edit" );
                edit_preferences_item = new JMenuItem( "Preferences ..." );
                edit_preferences_item.addActionListener( this );
            menu.add( edit_preferences_item );
                edit_legends_item = new JMenuItem( "Legends ..." );
                edit_legends_item.addActionListener( this );
            menu.add( edit_legends_item );

        super.add( menu );

            menu = new JMenu( "View" );
        super.add( menu );
            
            menu = new JMenu( "Help" );
                help_manuel_item = new JMenuItem( "Manuel" );
                help_manuel_item.addActionListener( this );
            menu.add( help_manuel_item );
                help_faq_item = new JMenuItem( "FAQ" );
                help_faq_item.addActionListener( this );
            menu.add( help_faq_item );
                help_about_item = new JMenuItem( "About" );
                help_about_item.addActionListener( this );
            menu.add( help_about_item );
        super.add( menu );

        // super.setAlignmentX( Component.LEFT_ALIGNMENT );
    }

    public void actionPerformed( ActionEvent evt )
    {
        Object evt_src = evt.getSource();
        if ( evt_src == this.file_exit_item ) {
            if ( isApplet )
                TopWindow.Legend.disposeAll();
            else
                TopWindow.First.disposeAll();
        }
        else if ( evt_src == this.file_select_item ) {
            LogFileChooser   file_chooser;
            int              istat;
            file_chooser = new LogFileChooser( isApplet );
            istat = file_chooser.showOpenDialog( TopWindow.First.getWindow() );
            if ( istat == JFileChooser.APPROVE_OPTION ) {
                String pathname;
                File   selected_file;
                selected_file = file_chooser.getSelectedFile();
                if ( selected_file != null )
                    pathname = selected_file.getPath();
                else
                    pathname = null;
                if ( pathname != null )
                    first_panel.getLogNameTextField().setText( pathname );
            }
            else
                JOptionPane.showMessageDialog( this, "No file chosen" );
        }
        else if ( evt_src == this.file_close_item ) {
            first_panel.getLogFileCloseButton().doClick();
        }
        else if ( evt_src == this.edit_legends_item ) {
            first_panel.getEditLegendButton().doClick();
        }
        else if ( evt_src == this.edit_preferences_item ) {
            first_panel.getEditPreferenceButton().doClick();
        }
        else if ( evt_src == this.help_manuel_item ) {
            URL manuel_URL = getURL( manuel_pathname );
            if ( manuel_URL != null ) {
                HTMLviewer  manuel_viewer;
                manuel_viewer = new HTMLviewer( manuel_URL );
                manuel_viewer.setVisible( true );
            }
            else
                Dialogs.warn( TopWindow.First.getWindow(),
                              "Cannot locate " + manuel_pathname + "." );
        }
        else if ( evt_src == this.help_faq_item ) {
            URL faq_URL = getURL( faq_pathname );
            if ( faq_URL != null ) {
                HTMLviewer  faq_viewer;
                faq_viewer = new HTMLviewer( faq_URL );
                faq_viewer.setVisible( true );
            }
            else
                Dialogs.warn( TopWindow.First.getWindow(),
                              "Cannot locate " + faq_pathname + "." );
        }
        else if ( evt_src == this.help_about_item ) {
            URL icon_URL = getURL( js_icon_pathname );
            if ( icon_URL != null ) {
                ImageIcon js_icon = new ImageIcon( icon_URL );
                Dialogs.info( TopWindow.First.getWindow(), about_str, js_icon );
            }
            else
                Dialogs.info( TopWindow.First.getWindow(), about_str, null );
        }
    }

    private URL getURL( String filename )
    {
        URL url = getClass().getResource( filename );
        return url;
    }
}
