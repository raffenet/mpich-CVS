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

import viewer.common.Dialogs;
import viewer.common.TopWindow;

public class ActionYaxisTreeCommit implements ActionListener
{
    private TimelineToolBar    toolbar;
    private ViewportTimeYaxis  canvas_vport;
    private YaxisMaps          y_maps;

    public ActionYaxisTreeCommit( TimelineToolBar    in_toolbar,
                                  ViewportTimeYaxis  in_canvas_vport,
                                  YaxisMaps          in_maps )
    {
        toolbar       = in_toolbar;
        canvas_vport  = in_canvas_vport;
        y_maps        = in_maps;
    }

    public void actionPerformed( ActionEvent event )
    {
        Debug.displayLine( "Action for Commit YaxisTree button, Redraw!" );

        if ( ! y_maps.update() )
            Dialogs.error( TopWindow.Timeline.getWindow(),
                           "Error in updating YaxisMaps!" );
        // y_maps.printMaps( System.out );
        canvas_vport.fireComponentResized();

        /*
           There are too many occasion that need to redraw timelines canvas.
           Leave commit_btn enabled all the time. 1/18/2002
           Set toolbar buttons to reflect status
        */
        // toolbar.commit_btn.setEnabled( false );
    }
}
