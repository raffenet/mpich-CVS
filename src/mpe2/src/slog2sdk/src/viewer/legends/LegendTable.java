/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.legends;

import java.util.Map;
import java.awt.Insets;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Component;
import javax.swing.SwingConstants;
import javax.swing.JComponent;
import javax.swing.Icon;
import javax.swing.JLabel;
import javax.swing.JCheckBox;
import javax.swing.JTable;
import javax.swing.table.TableColumn;
import javax.swing.table.TableColumnModel;
import javax.swing.table.TableCellRenderer;
import javax.swing.table.DefaultTableCellRenderer;

import base.drawable.Category;
import logformat.slog2.CategoryMap;

public class LegendTable extends JTable
{
    private static final Insets EMPTY_INSETS
                                = new Insets( 0, 0, 0, 0 );
    private static final Color  CELL_BACKCOLOR
                                = Const.CELL_BACKCOLOR;
    private static final Color  CELL_FORECOLOR
                                = Const.CELL_FORECOLOR;
    private static final Color  CELL_BACKCOLOR_SELECTED
                                = Const.CELL_BACKCOLOR_SELECTED;
    private static final Color  CELL_FORECOLOR_SELECTED
                                = Const.CELL_FORECOLOR_SELECTED;

    private LegendTableModel    table_model;
    private TableColumnModel    column_model;

    public LegendTable( CategoryMap  map )
    {
        super();

        table_model = new LegendTableModel( map );
        super.setModel( table_model );
        super.setDefaultRenderer( CategoryIcon.class,
                                  new CategoryIconRenderer() );
        super.setDefaultEditor( CategoryIcon.class,
                                new CategoryIconEditor() );
        super.setAutoResizeMode( AUTO_RESIZE_OFF );
        super.setIntercellSpacing( new Dimension( 2, 2 ) );

        column_model  = super.getColumnModel();
        this.setColumnHeaderRenderers();
        this.initColumnSize();

        // super.getSelectionModel().addListSelectionListener( table_model );
    }

    private void setColumnHeaderRenderers()
    {
        TableColumn        column; 
        JLabel             header;
        Color              bg_color;
        int                column_count;

        column_count  = table_model.getColumnCount();
        for ( int icol = 0; icol < column_count; icol++ ) {
            column     = column_model.getColumn( icol );
            header     = (JLabel) column.getHeaderRenderer();
            if ( header == null ) {
                header     = new DefaultTableCellRenderer();
                header.setText( table_model.getColumnName( icol ) );
                header.setToolTipText( table_model.getColumnToolTip( icol ) );
                header.setHorizontalAlignment( SwingConstants.CENTER );
                header.setBackground( Color.gray );
                header.setForeground( Color.white );
                column.setHeaderRenderer( (TableCellRenderer) header );
            }
            else
                header.setToolTipText( table_model.getColumnToolTip( icol ) );
        }
    }

    private void initColumnSize()
    {
        TableCellRenderer  renderer;
        Component          component;
        TableColumn        column; 
        Dimension          cell_size;
        Insets             cell_insets;
        int                column_count, cell_height, row_count, row_height;
        int                vport_width, vport_height;

        vport_width   = 0;
        vport_height  = 0;

        row_height    = 0;
        column_count  = table_model.getColumnCount();
        for ( int icol = 0; icol < column_count; icol++ ) {
            renderer   = super.getDefaultRenderer(
                               table_model.getColumnClass( icol ) );
            component  = renderer.getTableCellRendererComponent( this,
                                  table_model.getColumnTypicalValue( icol ),
                                  false, false, 0, icol );
            if ( component instanceof CategoryIconRenderer )
                cell_insets = ( (JComponent) component ).getInsets();
            else
                cell_insets = EMPTY_INSETS;
            cell_size  = component.getPreferredSize();
            column     = column_model.getColumn( icol );
            column.setPreferredWidth( cell_size.width
                                    + cell_insets.left + cell_insets.right );
            vport_width  += cell_size.width;
            cell_height   = cell_size.height
                          + cell_insets.top + cell_insets.bottom;
            if ( cell_height > row_height )
                row_height  = cell_height;
        }
        super.setRowHeight( row_height );

        row_count     = table_model.getRowCount();
        if ( row_count > Const.LIST_MAX_VISIBLE_ROW_COUNT )
            vport_height  = row_height * Const.LIST_MAX_VISIBLE_ROW_COUNT;
        else
            vport_height  = row_height * row_count;
        super.setPreferredScrollableViewportSize(
              new Dimension( vport_width, vport_height ) );
    }

    private void toggleCheckboxesAtColumn( int icolumn )
    {
        int[]      irows;
        int        irow, idx;
        Boolean    bval;

        irows  = super.getSelectedRows();
        for ( idx = 0; idx < irows.length; idx++ ) {
             irow  = irows[ idx ];
             bval  = (Boolean) table_model.getValueAt( irow, icolumn );
             if ( bval.booleanValue() )
                 table_model.setValueAt( Boolean.FALSE, irow, icolumn );
             else
                 table_model.setValueAt( Boolean.TRUE, irow, icolumn );
        }
    }

    public void toggleVisibilityCheckboxes()
    {
         this.toggleCheckboxesAtColumn( LegendTableModel.VISIBILITY_COLUMN );
    }

    public void toggleSearchabilityCheckboxes()
    {
         this.toggleCheckboxesAtColumn( LegendTableModel.SEARCHABILITY_COLUMN );
    }
}
