/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.awt.Container;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import javax.swing.*;

public class PreferenceFrame extends JFrame
                             implements ActionListener
{
    private PreferencePanel  pptys_panel;

    private JButton          update_btn;
    private JButton          save_btn;
    private JButton          close_btn;

    public PreferenceFrame()
    {
        super( "Preferences" );
        super.setDefaultCloseOperation( WindowConstants.DO_NOTHING_ON_CLOSE );
        TopWindow.Preference.disposeAll();
        TopWindow.Preference.setWindow( this );

        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        pptys_panel = new PreferencePanel();
        pptys_panel.updateAllFields();
        root_panel.add( pptys_panel );

        JPanel mid_panel = new JPanel();
        mid_panel.setLayout( new BoxLayout( mid_panel, BoxLayout.X_AXIS ) );
            mid_panel.add( Box.createHorizontalGlue() );

            update_btn = new JButton( "update" );
            update_btn.setToolTipText(
            "Update all parameters based on the current preference" );
            // update_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            update_btn.addActionListener( this );
            mid_panel.add( update_btn );

            mid_panel.add( Box.createHorizontalGlue() );

            save_btn = new JButton( "save" );
            save_btn.setToolTipText(
            "Save preference to Jumpshot-4 setup file" );
            // save_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            save_btn.addActionListener( this );
            mid_panel.add( save_btn );

            mid_panel.add( Box.createHorizontalGlue() );
        root_panel.add( mid_panel );

        JPanel end_panel = new JPanel();
        end_panel.setLayout( new BoxLayout( end_panel, BoxLayout.X_AXIS ) );
            end_panel.add( Box.createHorizontalGlue() );

            close_btn = new JButton( "close" );
            close_btn.setToolTipText( "Close this window" );
            // close_btn.setAlignmentY( Component.CENTER_ALIGNMENT );
            close_btn.addActionListener( this );
            end_panel.add( close_btn );

            end_panel.add( Box.createHorizontalGlue() );
        root_panel.add( end_panel );

        addWindowListener( new WindowAdapter() {
            public void windowClosing( WindowEvent e ) {
                setVisible( false );
            }
        } );
    }

    public void updateAllParameters()
    {
        pptys_panel.updateAllParameters();
    }

    public void actionPerformed( ActionEvent evt )
    {
        if ( evt.getSource() == this.update_btn ) {
            pptys_panel.updateAllParameters();
        }
        else if ( evt.getSource() == this.save_btn ) {
            pptys_panel.updateAllParameters();
            Parameters.writeToSetupFile( this );
        }
        else if ( evt.getSource() == this.close_btn ) {
            setVisible( false );
        }
    }
}
