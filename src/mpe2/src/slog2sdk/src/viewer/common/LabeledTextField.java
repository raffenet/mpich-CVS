/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.text.NumberFormat;
import java.text.DecimalFormat;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Dimension;
import java.awt.event.ActionListener;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.text.Document;
import javax.swing.text.BadLocationException;
import javax.swing.*;

public class LabeledTextField extends JPanel
{
    private static final   int     TEXT_HEIGHT = 20;
    private static         Font    FONT        = null;

    private JLabel                 tag;
    private JTextField             fld;
    private DecimalFormat          fmt;
    // private int                    preferred_height;

    private FieldDocumentListener  self_listener;

    public LabeledTextField( String label, String format )
    {
        super();
        setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );
        tag = new JLabel( label );
        fld = new JTextField();
        tag.setLabelFor( fld );
        add( tag );
        add( fld );

        // preferred_height = fld.getPreferredSize().height + this.TEXT_HEIGHT;
        if ( format != null ) {
            fmt = (DecimalFormat) NumberFormat.getInstance();
            fmt.applyPattern( format );
            fld.setColumns( this.getCalibratedColumnWidth( format.length() ) );
        }
        else
            fmt = null;

        /*  No self DocumentListener by default  */
        self_listener = null;

        // tag.setBorder( BorderFactory.createEtchedBorder() );
        fld.setBorder( BorderFactory.createEtchedBorder() );

        if ( FONT != null ) {
            tag.setFont( FONT );
            fld.setFont( FONT );
        }
    }

    public static void setDefaultFont( Font font )
    {
        FONT = font;
    }

    public void setLabelFont( Font font )
    {
        tag.setFont( font );
    }

    public void setFieldFont( Font font )
    {
        fld.setFont( font );
    }

    public void setHorizontalAlignment( int alignment )
    {
        fld.setHorizontalAlignment( alignment );
    }

    //  JTextField.getColumnWidth() uses char('m') defines column width
    //  getCalibratedColumnWidth() computes the effective char width
    //  that is needed by the JTextField constructor
    private int getCalibratedColumnWidth( int num_numeric_columns )
    {
        FontMetrics metrics;
        int         num_char_columns;

        metrics = fld.getFontMetrics( fld.getFont() );
        num_char_columns = (int) Math.ceil( (double) num_numeric_columns
                                          * metrics.charWidth( '8' )
                                          / metrics.charWidth( 'm' ) );
        // System.out.println( "num_char_columns = " + num_char_columns );
        return num_char_columns;
    }

    public void setText( String str )
    {
        fld.setText( str );
    }

    public String getText()
    {
        if ( self_listener != null )
            return self_listener.getLastUpdatedText();
        else
            return fld.getText();
    }

    public void setBoolean( boolean bval )
    {
        fld.setText( String.valueOf( bval ) );
    }

    public boolean getBoolean()
    {
        String  bool_str = null;
        if ( self_listener != null )
            bool_str = self_listener.getLastUpdatedText();
        else
            bool_str = fld.getText();
        return    bool_str.equalsIgnoreCase( "true" )
               || bool_str.equalsIgnoreCase( "yes" );
    }

    public void setShort( short sval )
    {
        fld.setText( fmt.format( sval ) );
    }

    public short getShort()
    {
        if ( self_listener != null )
            return Short.parseShort( self_listener.getLastUpdatedText() );
        else
            return Short.parseShort( fld.getText() );
    }

    public void setInteger( int ival )
    {
        // fld.setText( Integer.toString( ival ) );
        fld.setText( fmt.format( ival ) );
    }

    public int getInteger()
    {
        if ( self_listener != null )
            return Integer.parseInt( self_listener.getLastUpdatedText() );
        else
            return Integer.parseInt( fld.getText() );
    }

    public void setFloat( float fval )
    {
        fld.setText( fmt.format( fval ) );
    }

    public float getFloat()
    {
        if ( self_listener != null )
            return Float.parseFloat( self_listener.getLastUpdatedText() );
        else
            return Float.parseFloat( fld.getText() );
    }

    public void setDouble( double dval )
    {
        fld.setText( fmt.format( dval ) );
    }

    public double getDouble()
    {
        if ( self_listener != null )
            return Double.parseDouble( self_listener.getLastUpdatedText() );
        else
            return Double.parseDouble( fld.getText() );
    }

    public void setEditable( boolean flag )
    {
        fld.setEditable( flag );
    }

    public void addActionListener( ActionListener listener )
    {
        fld.addActionListener( listener );
    }

    public void addSelfDocumentListener()
    {
        self_listener = new FieldDocumentListener();
        fld.getDocument().addDocumentListener( self_listener );    
    }

    // BoxLayout respects component's maximum size
    public Dimension getMaximumSize()
    {
        // return new Dimension( Short.MAX_VALUE, preferred_height );
        return new Dimension( Short.MAX_VALUE, 
                              fld.getPreferredSize().height
                            + this.TEXT_HEIGHT );
    }



    /*  DocumentListener Interface  */
    private class FieldDocumentListener implements DocumentListener
    {
        private Document            last_updated_document;

        public FieldDocumentListener()
        {
            last_updated_document  = null;
        }

        public void changedUpdate( DocumentEvent evt )
        {
            last_updated_document = evt.getDocument();
        }

        public void insertUpdate( DocumentEvent evt )
        {
            last_updated_document = evt.getDocument();
        }

        public void removeUpdate( DocumentEvent evt )
        {
            last_updated_document = evt.getDocument();
        }

        public String getLastUpdatedText()
        {
            if ( last_updated_document != null ) {
                try {
                    return last_updated_document.getText( 0,
                           last_updated_document.getLength() );
                } catch ( BadLocationException err ) {
                    return null;
                }
            }
            return null;
        }
    }
}
