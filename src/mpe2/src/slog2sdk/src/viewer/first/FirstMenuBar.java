/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.first;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.border.EtchedBorder;

import viewer.common.TopWindow;

public class FirstMenuBar extends JMenuBar
                          implements ActionListener
{
    private        boolean         isApplet;
    private        FirstPanel      first_panel;

    private        JMenuItem       file_select_item;
    private        JMenuItem       file_close_item;
    private        JMenuItem       file_exit_item;
    private        JMenuItem       edit_legends_item;
    private        JMenuItem       edit_preferences_item;
    private        JMenuItem       help_manuel_item;
    private        JMenuItem       help_faq_item;
    private        JMenuItem       help_about_item;

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
            first_panel.getLogFileSelectButton().doClick();
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
            first_panel.getHelpManuelButton().doClick();
        }
        else if ( evt_src == this.help_faq_item ) {
            first_panel.getHelpFAQsButton().doClick();
        }
        else if ( evt_src == this.help_about_item ) {
            first_panel.getHelpAboutButton().doClick();
        }
    }
}
