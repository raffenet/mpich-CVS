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
import java.net.*;
import javax.swing.*;

public class ActionSearch implements ActionListener
{
    private TimelineToolBar    toolbar;
    private ViewportTimeYaxis  canvas_vport;

    public ActionSearch( TimelineToolBar   in_toolbar,
                         ViewportTimeYaxis in_vport )
    {
        toolbar       = in_toolbar;
        canvas_vport  = in_vport;
    }

    public void actionPerformed( ActionEvent event )
    {
        Object evt_src = event.getSource();
        if ( evt_src == toolbar.searchInit_btn ) {
        }
        else if ( evt_src == toolbar.searchBack_btn ) {
        }
        else if ( evt_src == toolbar.searchFore_btn ) {
        }

        if ( Debug.isActive() )
            Debug.println( "Action for Serach button. " );
    }
}
