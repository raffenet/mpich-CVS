/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import javax.swing.*;

public class PreferencePanel extends JPanel
{
    private static int                    VERTICAL_GAP_HEIGHT = 10;

    private        LabeledTextField       fld_Y_AXIS_ROOT_LABEL;
    private        LabeledTextField       fld_AUTO_WINDOWS_LOCATION;
    private        LabeledTextField       fld_SCREEN_HEIGHT_RATIO;
    private        LabeledTextField       fld_INIT_SLOG2_LEVEL_READ;

    private        LabeledTextField       fld_Y_AXIS_ROOT_VISIBLE;
    private        LabeledTextField       fld_Y_AXIS_ROW_HEIGHT;
    private        LabeledTextField       fld_STATE_HEIGHT_FACTOR;
    private        LabeledTextField       fld_NESTING_HEIGHT_FACTOR;
    private        LabeledTextField       fld_ARROW_HEAD_LENGTH;
    private        LabeledTextField       fld_ARROW_HEAD_HALF_WIDTH;
    private        LabeledTextField       fld_CLICK_RADIUS_TO_LINE;


    public PreferencePanel()
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        JLabel label;
        label = new JLabel( "Options for all windows" );
        label.setToolTipText( "Options become effective after being saved "
                            + "and the program is restarted" );
        super.add( label );

        fld_Y_AXIS_ROOT_LABEL = new LabeledTextField(
                                    "Y_AXIS_ROOT_LABEL",
                                    Const.STRING_FORMAT );
        fld_Y_AXIS_ROOT_LABEL.setToolTipText(
        "Label for the root node of the Y-axis tree label in the left panel" );
        fld_Y_AXIS_ROOT_LABEL.setHorizontalAlignment( JTextField.CENTER );
        fld_Y_AXIS_ROOT_LABEL.addSelfDocumentListener();
        fld_Y_AXIS_ROOT_LABEL.setEditable( true );
        super.add( fld_Y_AXIS_ROOT_LABEL );

        fld_AUTO_WINDOWS_LOCATION = new LabeledTextField(
                                        "AUTO_WINDOWS_LOCATION",
                                        Const.BOOLEAN_FORMAT );
        fld_AUTO_WINDOWS_LOCATION.setToolTipText(
        "Whelther to let Jumpshot-4 automatically set windows placement." );
        fld_AUTO_WINDOWS_LOCATION.setHorizontalAlignment( JTextField.CENTER );
        fld_AUTO_WINDOWS_LOCATION.addSelfDocumentListener();
        fld_AUTO_WINDOWS_LOCATION.setEditable( true );
        super.add( fld_AUTO_WINDOWS_LOCATION );

        fld_SCREEN_HEIGHT_RATIO = new LabeledTextField(
                                      "SCREEN_HEIGHT_RATIO",
                                      Const.FLOAT_FORMAT );
        fld_SCREEN_HEIGHT_RATIO.setToolTipText(
        "Initial available screen height(ratio) for Timeline window's canvas" );
        fld_SCREEN_HEIGHT_RATIO.setHorizontalAlignment( JTextField.CENTER );
        fld_SCREEN_HEIGHT_RATIO.addSelfDocumentListener();
        fld_SCREEN_HEIGHT_RATIO.setEditable( true );
        super.add( fld_SCREEN_HEIGHT_RATIO );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        fld_INIT_SLOG2_LEVEL_READ = new LabeledTextField(
                                        "INIT_SLOG2_LEVEL_READ",
                                        Const.SHORT_FORMAT );
        fld_INIT_SLOG2_LEVEL_READ.setToolTipText(
          "The number of SLOG-2 level read into memory when Jumpshot-4 starts, "
        + "it also asymptotically affects the performance of zooming in/out." );
        fld_INIT_SLOG2_LEVEL_READ.setHorizontalAlignment( JTextField.CENTER );
        fld_INIT_SLOG2_LEVEL_READ.addSelfDocumentListener();
        fld_INIT_SLOG2_LEVEL_READ.setEditable( true );
        super.add( fld_INIT_SLOG2_LEVEL_READ );

        super.add( Box.createVerticalStrut( 2 * VERTICAL_GAP_HEIGHT ) );

        /*                                                        */

        label = new JLabel( "Options for Timeline window" );
        label.setToolTipText( "Options become effective after return "
                            + "and the Timeline window is redrawn" );
        super.add( label );

        fld_Y_AXIS_ROOT_VISIBLE = new LabeledTextField(
                                      "Y_AXIS_ROOT_VISIBLE",
                                      Const.BOOLEAN_FORMAT );
        fld_Y_AXIS_ROOT_VISIBLE.setToolTipText(
        "Whelther to show the top of the Y-axis tree-styled directory label." );
        fld_Y_AXIS_ROOT_VISIBLE.setHorizontalAlignment( JTextField.CENTER );;
        fld_Y_AXIS_ROOT_VISIBLE.addSelfDocumentListener();
        fld_Y_AXIS_ROOT_VISIBLE.setEditable( true );
        super.add( fld_Y_AXIS_ROOT_VISIBLE );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        fld_Y_AXIS_ROW_HEIGHT = new LabeledTextField(
                                    "Y_AXIS_ROW_HEIGHT",
                                    Const.INTEGER_FORMAT );
        fld_Y_AXIS_ROW_HEIGHT.setToolTipText(
        "Row height of Y-axis tree in pixel, i.e. height for each timeline." );
        fld_Y_AXIS_ROW_HEIGHT.setHorizontalAlignment( JTextField.CENTER );;
        fld_Y_AXIS_ROW_HEIGHT.addSelfDocumentListener();
        fld_Y_AXIS_ROW_HEIGHT.setEditable( true );
        super.add( fld_Y_AXIS_ROW_HEIGHT );

        fld_STATE_HEIGHT_FACTOR = new LabeledTextField(
                                      "STATE_HEIGHT_FACTOR",
                                      Const.FLOAT_FORMAT );
        fld_STATE_HEIGHT_FACTOR.setToolTipText(
          "Factor f, 0.0 < f < 1.0, defines the height of the outermost "
        + "rectangle with respect to the Y_AXIS_ROW_HEIGHT. The larger "
        + "the factor f is, the larger the outermost rectangle will be." );
        fld_STATE_HEIGHT_FACTOR.setHorizontalAlignment( JTextField.CENTER );
        fld_STATE_HEIGHT_FACTOR.addSelfDocumentListener();
        fld_STATE_HEIGHT_FACTOR.setEditable( true );
        super.add( fld_STATE_HEIGHT_FACTOR );

        fld_NESTING_HEIGHT_FACTOR = new LabeledTextField(
                                        "NESTING_HEIGHT_FACTOR",
                                        Const.FLOAT_FORMAT );
        fld_NESTING_HEIGHT_FACTOR.setToolTipText(
          "Factor f, 0.0 < f < 1.0, defines the gap between nesting rectangles."
        + " The larger the factor f is, the smaller the gap." );
        fld_NESTING_HEIGHT_FACTOR.setHorizontalAlignment( JTextField.CENTER );
        fld_NESTING_HEIGHT_FACTOR.addSelfDocumentListener();
        fld_NESTING_HEIGHT_FACTOR.setEditable( true );
        super.add( fld_NESTING_HEIGHT_FACTOR );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        fld_ARROW_HEAD_LENGTH = new LabeledTextField(
                                    "ARROW_HEAD_LENGTH",
                                    Const.INTEGER_FORMAT );
        fld_ARROW_HEAD_LENGTH.setToolTipText(
        "Length of arrow head in pixel." );
        fld_ARROW_HEAD_LENGTH.setHorizontalAlignment( JTextField.CENTER );
        fld_ARROW_HEAD_LENGTH.addSelfDocumentListener();
        fld_ARROW_HEAD_LENGTH.setEditable( true );
        super.add( fld_ARROW_HEAD_LENGTH );

        fld_ARROW_HEAD_HALF_WIDTH = new LabeledTextField(
                                        "ARROW_HEAD_HALF_WIDTH",
                                        Const.INTEGER_FORMAT );
        fld_ARROW_HEAD_HALF_WIDTH.setToolTipText(
        "Half width of arrow head's base in pixel." );
        fld_ARROW_HEAD_HALF_WIDTH.setHorizontalAlignment( JTextField.CENTER );
        fld_ARROW_HEAD_HALF_WIDTH.addSelfDocumentListener();
        fld_ARROW_HEAD_HALF_WIDTH.setEditable( true );
        super.add( fld_ARROW_HEAD_HALF_WIDTH );

        fld_CLICK_RADIUS_TO_LINE = new LabeledTextField(
                                       "CLICK_RADIUS_TO_LINE",
                                       Const.INTEGER_FORMAT );
        fld_CLICK_RADIUS_TO_LINE.setToolTipText(
        "Pixel radius in which a point/click is considered on the arrow." );
        fld_CLICK_RADIUS_TO_LINE.setHorizontalAlignment( JTextField.CENTER );
        fld_CLICK_RADIUS_TO_LINE.addSelfDocumentListener();
        fld_CLICK_RADIUS_TO_LINE.setEditable( true );
        super.add( fld_CLICK_RADIUS_TO_LINE );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        super.setBorder( BorderFactory.createEtchedBorder() );
    }

    public void updateAllFields()
    {
        fld_Y_AXIS_ROOT_LABEL.setText( Parameters.Y_AXIS_ROOT_LABEL );
        fld_SCREEN_HEIGHT_RATIO.setFloat( Parameters.SCREEN_HEIGHT_RATIO );
        fld_INIT_SLOG2_LEVEL_READ.setShort( Parameters.INIT_SLOG2_LEVEL_READ );
        fld_AUTO_WINDOWS_LOCATION.setBoolean(
                                  Parameters.AUTO_WINDOWS_LOCATION );

        fld_Y_AXIS_ROOT_VISIBLE.setBoolean( Parameters.Y_AXIS_ROOT_VISIBLE );
        fld_Y_AXIS_ROW_HEIGHT.setInteger( Parameters.Y_AXIS_ROW_HEIGHT );
        fld_STATE_HEIGHT_FACTOR.setFloat( Parameters.STATE_HEIGHT_FACTOR );
        fld_NESTING_HEIGHT_FACTOR.setFloat( Parameters.NESTING_HEIGHT_FACTOR );
        fld_ARROW_HEAD_LENGTH.setInteger( Parameters.ARROW_HEAD_LENGTH );
        fld_ARROW_HEAD_HALF_WIDTH.setInteger(
                                  Parameters.ARROW_HEAD_HALF_WIDTH );
        fld_CLICK_RADIUS_TO_LINE.setInteger( Parameters.CLICK_RADIUS_TO_LINE );
    }

    public void updateAllParameters()
    {
        Parameters.Y_AXIS_ROOT_LABEL = fld_Y_AXIS_ROOT_LABEL.getText();
        Parameters.SCREEN_HEIGHT_RATIO = fld_SCREEN_HEIGHT_RATIO.getFloat();
        Parameters.INIT_SLOG2_LEVEL_READ = fld_INIT_SLOG2_LEVEL_READ.getShort();
        Parameters.AUTO_WINDOWS_LOCATION
                                     = fld_AUTO_WINDOWS_LOCATION.getBoolean();

        Parameters.Y_AXIS_ROOT_VISIBLE = fld_Y_AXIS_ROOT_VISIBLE.getBoolean();
        Parameters.Y_AXIS_ROW_HEIGHT = fld_Y_AXIS_ROW_HEIGHT.getInteger();
        Parameters.STATE_HEIGHT_FACTOR = fld_STATE_HEIGHT_FACTOR.getFloat();
        Parameters.NESTING_HEIGHT_FACTOR = fld_NESTING_HEIGHT_FACTOR.getFloat();
        Parameters.ARROW_HEAD_LENGTH = fld_ARROW_HEAD_LENGTH.getInteger();
        Parameters.ARROW_HEAD_HALF_WIDTH
                                     = fld_ARROW_HEAD_HALF_WIDTH.getInteger();
        Parameters.CLICK_RADIUS_TO_LINE = fld_CLICK_RADIUS_TO_LINE.getInteger();
    }
}
