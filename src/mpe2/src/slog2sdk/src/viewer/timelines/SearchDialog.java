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

public class SearchDialog extends JDialog 
                          implements ActionListener
{
    private JFrame       root_frame;
    private Container    root_panel;
    private JButton      close_btn;

    public SearchDialog( final JFrame  frame )
    {
        super( frame, "Search Box" );
        super.setDefaultCloseOperation( WindowConstants.HIDE_ON_CLOSE );

        root_frame = frame;

        root_panel = super.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        close_btn = new JButton( "close" );
        close_btn.addActionListener( this );
        close_btn.setAlignmentX( Component.CENTER_ALIGNMENT );

        super.setVisible( false );
    }

    private void setVisibleAtLocation( final Point global_pt )
    {
        this.setLocation( global_pt );
        this.pack();
        this.setVisible( true );
        this.toFront();
    }

    public void setVisibleAtDefaultLocation()
    {
        Rectangle  rect    = root_frame.getBounds();
        Point      loc_pt  = new Point( rect.x + rect.width / 2,
                                        rect.y + rect.height );
        this.setVisibleAtLocation( loc_pt );
    }

    public void replace( Component panel )
    {
        root_panel.removeAll();
        root_panel.add( panel );
        root_panel.add( close_btn );
        super.invalidate();
        super.validate();
    }

    public void actionPerformed( ActionEvent evt )
    {
        if ( evt.getSource() == close_btn )
            super.setVisible( false );
    }
}
