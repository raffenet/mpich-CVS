/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.convertor;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowAdapter;
import javax.swing.JFrame;
import javax.swing.JDialog;
import javax.swing.WindowConstants;

import viewer.common.LogFileChooser;

public class ConvertorDialog extends JDialog
{
    private static String          in_filename;      // For main()

    private        ConvertorPanel  top_panel;

    public ConvertorDialog( JFrame          ancestor_frame,
                            LogFileChooser  file_chooser )
    {
        // Make this a Modal Dialog
        super( ancestor_frame, "Logfile Convertor", true );
        super.setDefaultCloseOperation( WindowConstants.DO_NOTHING_ON_CLOSE );

        top_panel = new ConvertorPanel( file_chooser );
        super.setContentPane( top_panel );

        super.addWindowListener( new WindowAdapter() {
            public void windowClosing( WindowEvent evt ) {
                ConvertorDialog.this.setVisible( false );
                ConvertorDialog.this.dispose();
            }
        } );

        /* setVisible( true ) */;
    }

    public void init( String trace_filename )
    { top_panel.init( trace_filename ); }

    public void addActionListenerForCloseButton( ActionListener action )
    { top_panel.addActionListenerForCloseButton( action ); }

    public static String convertLogFile( JFrame          frame,
                                         LogFileChooser  chooser,
                                         String          filename )
    {
        ConvertorDialog        conv_dialog;
        CloseToRetrieveAction  retrieve_action;
        CloseToRetrieveAction  close2getname;
        conv_dialog    = new ConvertorDialog( frame, chooser );
        close2getname  = new CloseToRetrieveAction( conv_dialog );
        conv_dialog.top_panel.addActionListenerForCloseButton( close2getname );

        conv_dialog.pack();
        conv_dialog.init( filename );
        conv_dialog.setVisible( true );
        return close2getname.getFilename();
    }

    private static class CloseToRetrieveAction implements ActionListener
    {
        private ConvertorDialog  convertor;
        private String           filename;

        public CloseToRetrieveAction( ConvertorDialog convertor_dialog )
        { convertor  = convertor_dialog; }

        public String  getFilename()
        { return filename; }

        public void actionPerformed( ActionEvent evt )
        {
            filename = convertor.top_panel.getOutputSLOG2Name();
            convertor.setVisible( false );
            convertor.dispose();
        }
    }
}
