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

public class ActionSearchForward implements ActionListener
{
    private TimelineToolBar    toolbar;
    private ViewportTimeYaxis  canvas_vport;
    private SearchDialog       search_dialog;

    public ActionSearchForward( TimelineToolBar    in_toolbar,
                                ViewportTimeYaxis  in_vport,
                                SearchDialog       in_dialog )
    {
        toolbar       = in_toolbar;
        canvas_vport  = in_vport;
        search_dialog = in_dialog;
    }

    public void actionPerformed( ActionEvent event )
    {
        Component jcomponent = canvas_vport.searchForward();
        if ( jcomponent != null ) {
            search_dialog.replace( jcomponent );
            if ( ! search_dialog.isVisible() )
                search_dialog.setVisibleAtDefaultLocation();
        }
        else
            search_dialog.setVisible( false );

        if ( Debug.isActive() )
            Debug.println( "Action for Serach Forward button. " );
    }
}
