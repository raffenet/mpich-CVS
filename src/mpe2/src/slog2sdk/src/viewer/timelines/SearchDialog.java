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
import viewer.common.TopWindow;

public class SearchDialog extends JDialog 
                          implements ActionListener
{
    private JFrame             root_frame;
    private ViewportTimeYaxis  viewport;

    private Container          root_panel;
    private JButton            close_btn;

    public SearchDialog( final JFrame frame, ViewportTimeYaxis  vport )
    {
        super( frame, "Search Box" );
        super.setDefaultCloseOperation( WindowConstants.DO_NOTHING_ON_CLOSE );

        viewport   = vport;
        root_frame = frame;

        root_panel = super.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        close_btn = new JButton( "close" );
        close_btn.addActionListener( this );
        close_btn.setAlignmentX( Component.CENTER_ALIGNMENT );

        super.addWindowListener( new WindowAdapter()
        {
            public void windowClosing( WindowEvent evt )
            {
                SearchDialog.this.setVisible( false );
                viewport.eraseSearchedDrawable();
                viewport.repaint();
            }
        } );

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
        Rectangle rect   = null;
        Point     loc_pt = null;
        Frame     frame  = TopWindow.First.getWindow();
        if ( frame != null ) {
            rect    = frame.getBounds();
            loc_pt  = new Point( rect.x + rect.width, rect.y );
        }
        else {
            rect    = root_frame.getBounds();
            loc_pt  = new Point( rect.x + rect.width, rect.y );
        }
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
        if ( evt.getSource() == close_btn ) {
            super.setVisible( false );
            viewport.eraseSearchedDrawable();
            viewport.repaint();
        }
    }

}
