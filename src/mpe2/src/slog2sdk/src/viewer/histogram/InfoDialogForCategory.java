/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.histogram;

import java.awt.*;
import javax.swing.BoxLayout;
import javax.swing.tree.TreePath;

import base.drawable.CategoryWeight;
import viewer.zoomable.InfoDialog;

public class InfoDialogForCategory extends InfoDialog
{
    public InfoDialogForCategory( final Dialog          dialog, 
                                  final double          clicked_time,
                                  final String[]        y_colnames,
                                  final TreePath        ylabel_path,
                                  final CategoryWeight  clicked_twgt )
    {
        super( dialog, "Category Info Box", clicked_time );

        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        root_panel.add( new InfoPanelForCategory( y_colnames, ylabel_path,
                                                  clicked_twgt ) );

        root_panel.add( super.getCloseButtonPanel() );
    }
}
