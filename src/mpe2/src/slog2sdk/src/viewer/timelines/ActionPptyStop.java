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

import viewer.common.TopWindow;

public class ActionPptyStop implements ActionListener
{
    // private TimelinePanel top_panel;

    public ActionPptyStop( /* TimelinePanel panel */ )
    {
        // top_panel = panel;
    }

    public void actionPerformed( ActionEvent event )
    {
        if ( Debug.isActive() )
            Debug.println( "Action for Stop Property button" );
        TopWindow.Timeline.disposeAll();
    }
}
