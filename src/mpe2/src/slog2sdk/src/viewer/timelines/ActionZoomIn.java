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

import viewer.common.Const;
import viewer.common.Dialogs;

public class ActionZoomIn implements ActionListener
{
    private TimelineToolBar    toolbar;
    private ModelTime          model;
    private int                zoomlevel;

    public ActionZoomIn( TimelineToolBar in_toolbar, ModelTime in_model )
    {
        toolbar    = in_toolbar;
        model      = in_model;
        zoomlevel  = 0;
    }

    public void actionPerformed( ActionEvent event )
    {
        zoomlevel = model.getZoomLevel();
        if ( zoomlevel >= Const.MAX_ZOOM_LEVEL ) {
            Frame frame = (Frame) SwingUtilities.windowForComponent( toolbar );
            String msg = "The Current ZoomLevel(" + zoomlevel + ") exceeds "
                       + "the Maximum ZoomLevel(" + Const.MAX_ZOOM_LEVEL + ")!";
            Dialogs.error( frame, msg );
        }
        else
            model.zoomIn();

        /*
        // Set toolbar buttons to reflect status
        zoomlevel = model.getZoomLevel();
        if ( toolbar != null ) {
            toolbar.zoomIn_btn.setEnabled( zoomlevel < Const.MAX_ZOOM_LEVEL );
            toolbar.home_btn.setEnabled( zoomlevel != Const.MIN_ZOOM_LEVEL );
            toolbar.zoomOut_btn.setEnabled( zoomlevel > Const.MIN_ZOOM_LEVEL );
        }
        */

        if ( Debug.isActive() )
            Debug.println( "Action for Zoom In button. ZoomLevel = "
                         + zoomlevel );
    }
}
