/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.first;

import java.io.*;
import java.net.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.event.*;

import viewer.common.Dialogs;
import viewer.common.TopWindow;

public class HTMLviewer extends JDialog
                        implements ActionListener, HyperlinkListener
{
    private JTextField   input_fld;
    private JEditorPane  html_panel;
    private URL          docu_URL;

    private JButton      open_btn;

    public HTMLviewer( String title_str )
    {
        super( TopWindow.First.getWindow() );
        if ( title_str != null )
            setTitle( title_str );
        else
            setTitle( "HTML viewer" );
        setSize( 600, 300 );
        setBackground( Color.gray );
        getContentPane().setLayout( new BorderLayout() );

        JPanel top_panel = new JPanel();
        top_panel.setLayout( new BoxLayout( top_panel, BoxLayout.X_AXIS ) );
            JLabel URL_label = new JLabel( "URL : " );
            top_panel.add( URL_label ); 
            input_fld  = new JTextField();
            top_panel.add( input_fld );
            JButton close_btn = new JButton( "Close" );
            close_btn.addActionListener( this );
            top_panel.add( close_btn );
        getContentPane().add( top_panel, BorderLayout.NORTH );
        
        JScrollPane scroll_panel = new JScrollPane();
        scroll_panel.setBorder( BorderFactory.createLoweredBevelBorder() );
            html_panel = new JEditorPane();
            html_panel.setEditable( false );
        scroll_panel.getViewport().add( html_panel );
        getContentPane().add( scroll_panel, BorderLayout.CENTER );

        input_fld.addActionListener( this );
        html_panel.addHyperlinkListener( this );

        open_btn = null;

        addWindowListener( new WindowAdapter() {
            public void windowClosing( WindowEvent evt )
            { HTMLviewer.this.setVisible( false ); }
        } );
    }

    public HTMLviewer( String title_str, JButton button )
    {
        this( title_str );
        open_btn = button;
    }

    // Overriden setVisible to enable/disable the open_btm
    public void setVisible( boolean val )
    {
        super.setVisible( val );
        if ( open_btn != null )
            open_btn.setEnabled( !val );
    }

    public void init( URL init_URL )
    {
        docu_URL = init_URL;
        // System.out.println( "docu_URL = " + docu_URL );

        if ( docu_URL != null ) {
            input_fld.setText( docu_URL.toString() ); 
            this.updateHTMLpanel();
        }    
    }

    private void updateHTMLpanel()
    {
        try {
            docu_URL = new URL( input_fld.getText() );
            html_panel.setPage( docu_URL );
        } catch ( IOException ioerr ) {
            Dialogs.error( this, "Invalid URL: " + docu_URL.toString() );
        }
    }

    public void hyperlinkUpdate( final HyperlinkEvent evt )
    {
        if ( evt.getEventType() == HyperlinkEvent.EventType.ACTIVATED ) {
            html_panel.setCursor( new Cursor( Cursor.WAIT_CURSOR ) );
            SwingUtilities.invokeLater( new Runnable() 
            {
                public void run() {
                    Document docu = html_panel.getDocument();
                    try {
                        docu_URL   = evt.getURL();
                        if ( docu_URL != null ) {
                            // System.out.println( "  docu_URL = " + docu_URL );
                            input_fld.setText( docu_URL.toString() );
                            html_panel.setPage( docu_URL );
                        }
                        else {
                            Dialogs.error( HTMLviewer.this,
                                           "Invalid Link: NULL pointer!" ); 
                            html_panel.setDocument( docu );
                        }
                    } catch ( IOException ioerr ) {
                        Dialogs.error( HTMLviewer.this,
                                       "Invalid Link: " + docu_URL.toString() );
                    }
                    html_panel.setCursor( new Cursor( Cursor.DEFAULT_CURSOR ) );
                }
            } );
        }
    } 

    public void actionPerformed( ActionEvent evt )
    {
        if ( evt.getActionCommand().equals( "Close" ) )
            this.setVisible( false );
        else
            this.updateHTMLpanel();
    }

    public static void main( String args[] )
    {
        URL init_URL = ClassLoader.getSystemResource( "doc/html/index.html" );
        HTMLviewer htmlview = new HTMLviewer( null );
        htmlview.init( init_URL );
        htmlview.setVisible( true );
    }
}
