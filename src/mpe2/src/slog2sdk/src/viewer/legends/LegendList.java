/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.legends;

import java.util.Map;
import java.util.Vector;
import java.util.Collection;
import java.util.Iterator;
import java.awt.Component;
import java.awt.Color;
import javax.swing.Icon;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.ListCellRenderer;
import javax.swing.BorderFactory;
import javax.swing.SwingConstants;
import javax.swing.border.Border;

import base.drawable.Category;
import logformat.slog2.CategoryMap;

public class LegendList extends JList
{
    private static final Color  CELL_BACKCOLOR
                                = Const.CELL_BACKCOLOR;
    private static final Color  CELL_FORECOLOR
                                = Const.CELL_FORECOLOR;
    private static final Color  CELL_BACKCOLOR_SELECTED
                                = Const.CELL_BACKCOLOR_SELECTED;
    private static final Color  CELL_FORECOLOR_SELECTED
                                = Const.CELL_FORECOLOR_SELECTED;

    public LegendList( CategoryMap  map )
    {
        super();

        LegendCell  cell;
        Category    objdef;
        Vector      cells    = new Vector( map.size() );
        Iterator    objdefs  = map.values().iterator();
        while ( objdefs.hasNext() ) {
            objdef = (Category) objdefs.next();
            cell   = new LegendCell( objdef );
            cells.add( cell );
        }
        super.setListData( cells );
        if ( cells.size() < Const.LIST_MAX_VISIBLE_ROW_COUNT )
            super.setVisibleRowCount( cells.size() );
        else
            super.setVisibleRowCount( Const.LIST_MAX_VISIBLE_ROW_COUNT );
        super.setFixedCellHeight( Const.CELL_HEIGHT );
        super.setCellRenderer( new LegendCellListRenderer() );
    }

    private class LegendCell
    {
        private Category  type;
        private Icon      icon;
        private String    text;

        public LegendCell( Category new_type )
        {
            type = new_type;
            text = type.getName();
            icon = new CategoryIcon( type );
        }

        public void setCategoryVisible( boolean new_value )
        {
            type.setVisible( new_value );
        }

        public String getText()
        {
            return text;
        }

        public Icon getIcon()
        {
            return icon;
        }

        public String toString()
        {
            return text;
        }
    }

    private class LegendCellListRenderer extends JLabel
                                         implements ListCellRenderer
    {
        private Border  raised_border, lowered_border;

        public LegendCellListRenderer()
        {
            super();
            super.setOpaque( true );
            super.setIconTextGap( Const.CELL_ICON_TEXT_GAP );
            raised_border  = BorderFactory.createRaisedBevelBorder();
            lowered_border = BorderFactory.createLoweredBevelBorder();
        }

        public Component getListCellRendererComponent( JList list,
                                                       Object value,
                                                       int index,
                                                       boolean isSelected,
                                                       boolean cellHasFocus )
        {
            LegendCell cell = (LegendCell) value;
            super.setText( cell.getText() );
            super.setIcon( cell.getIcon() );
            if ( isSelected ) {
                super.setForeground( CELL_FORECOLOR_SELECTED );
                super.setBackground( CELL_BACKCOLOR_SELECTED );
                super.setBorder( lowered_border );
            }
            else {
                super.setForeground( CELL_FORECOLOR );
                super.setBackground( CELL_BACKCOLOR );
                super.setBorder( raised_border );
            }
            cell.setCategoryVisible( ! isSelected );
            repaint();
            return this;
        }
    }
}
