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

import base.drawable.Drawable;
import viewer.legends.CategoryLabel;

public class DrawableInfoDialog extends JDialog
                                implements ActionListener
{
    private static final Component  STRUT = Box.createHorizontalStrut( 5 );
    private static final Component  GLUE  = Box.createHorizontalGlue();

    public DrawableInfoDialog( final Frame  frame, final Drawable  dobj )
    {
        super( frame, "Drawable Info Box" );
        // super.setDefaultCloseOperation( WindowConstants.DISPOSE_ON_CLOSE );
        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

            JPanel     top_panel = new JPanel();
            top_panel.setLayout( new BoxLayout( top_panel, BoxLayout.X_AXIS ) );
            CategoryLabel  label = new CategoryLabel( dobj.getCategory() );
            top_panel.add( STRUT );
            top_panel.add( label );
            top_panel.add( GLUE );
        root_panel.add( top_panel );

            JTextArea  text_area = new JTextArea( dobj.toString(), 5, 20 );
            text_area.setLineWrap( true );
        root_panel.add( new JScrollPane( text_area ) );

            JButton    close_btn = new JButton( "close" );
            close_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            close_btn.addActionListener( this );
        root_panel.add( close_btn );

        addWindowListener( new WindowAdapter() {
            public void windowClosing( WindowEvent e ) {
                dispose();
            }
        } );
    }

    public void setVisibleAtLocation( final Point pt )
    {
        this.setLocation( pt );
        this.pack();
        this.setVisible( true );
        this.toFront();
    }

    public void actionPerformed( ActionEvent evt )
    {
        super.dispose();
    }
}
