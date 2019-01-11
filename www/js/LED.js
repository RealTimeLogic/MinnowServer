/*
  Code for dynamically emitting (creating HTML) for LED light and LED
  switch, including code for interacting with the LED HTML UI.
*/


/* The global LED object is used for all LED creation and interaction.

   Functions:
   led.emit(name, ledID, color, on) -- Emit (create and insert) LED HTML
   led.click(callback) -- Install click event, where callback(ledID, on)
   led.set(ledID, on) -- Set led with ID on/off
   led.reset() -- Remove all LEDs (reset/remove HTML)
 */
   
const led={};


/*
  The complete LED and switch is named a 'led-section'. Each
  emitted 'led-section' is appended to the HTML section whose ID is
  'led-panel'. See index.html for details.

  The following example shows how a red LED with ledID 5 is emitted in
  'off' mode. The ledID is a number received from the server and is
  used as the ID number when sending on/off states to the server and
  vice versa.

      <!-- RED LED -->
      <div class='led-section'>

        <!-- LED TAG -->
          <span class='led-tag'>
            <span class='tag-content'>LED1</span>
          </span>
        <!-- END LED TAG -->

        <!-- LED LIGHT -->
        <span id='ledlight-5' class='led-light led-red led-off'></span>
        <!-- END LED LIGHT -->

        <!-- LED SWITCH -->
        <label class='led-light-switch'>
          <input id='ledswitch-5' type='checkbox'>
          <span class='switch-slider'>
            <span class='switch-text on'>ON</span>
            <span class='switch-text off'>OFF</span>
          </span>
        </label>
        <!-- END LED SWITCH -->

      </div>
      <!-- END RED LED -->

  Notice how we set the LED to off by adding the class 'led-off' to
  the LED element in the above HTML example. We also insert the ledID
  into the HTML by adding an 'id' tag for the LED and LED switch. See
  'ledlight-5' and 'ledswitch-5' above. The id tag makes it easy to
  find the elements by JS and to dynamically update this elements when
  clicked.
*/
(()=>{

/* 
   The LED template (see above comment for HTML example).
   An ES6 template string can span multiple lines:
   https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals
   We are also instrumenting the template with {tags} so it can be
   used with the printf like formatString function.
*/
const ledTemplate=`
<div class='led-section'>
    <span class='led-tag'>
    <span class='tag-content'>{0}</span>
    </span>
  <span id='ledlight-{1}' class='led-light led-{2} {3}'></span>
  <label class='led-light-switch'>
    <input id='ledswitch-{4}' type='checkbox' {5}>
    <span class='switch-slider'>
      <span class='switch-text on'>ON</span>
      <span class='switch-text off'>OFF</span>
    </span>
  </label>
</div>
`;

/* formatString('You have {0} cars and {1} bikes', 'two', 'three')
 * returns 'You have two cars and three bikes'
*/
const formatString = (str, ...params) => {
    for (let i = 0; i < params.length; i++) {
        var reg = new RegExp("\\{" + i + "\\}", "gm");
        str = str.replace(reg, params[i]);
    }
    return str;
};

/* Emit LED HTML code
*/
led.emit = (name, id, color, on) => {
    $("#led-panel").append(formatString(
        ledTemplate,
        name,
        id,
        color,
        on ? "" : "led-off",
        id,
        on ? "" : "checked"));
};

/* Add click event to all emitted LED switches (the checkbox input element).
   This function must be called one time after emitting all LEDs
   arg: callback(ledID, on)
*/
led.click = (callback) => {
    $(".led-light-switch > input").on("click",function() {
        // Extract the number from 'ledswitch-number'
        const ledID=$(this).attr("id").match(/(\d+)/)[0];
        const on = this.checked ? false : true; // Inverse of 'checked'
        callback(parseInt(ledID), on);
    });
};

led.set = (ledID, on) => {
    // JS functions can be accessed as obj.func() or obj["func"]()
    $("#ledlight-"+ledID)[on ? "removeClass" : "addClass"]("led-off");
    $("#ledswitch-"+ledID)[0].checked = ! on; // Checked = off (inverse)
};

led.reset = () => $("#led-panel").html("");

})();
