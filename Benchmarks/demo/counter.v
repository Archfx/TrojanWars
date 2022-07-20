module up_counter( clk, reset, text, counter);

	input clk, reset;
	input[3:0] text;
	output[3:0] counter;
		

    reg [3:0] counter_up;

    always @(posedge clk or posedge reset)
    begin
        if(reset)
            counter_up <= 4'd0;
    else
        counter_up <= counter_up + 4'd1;
    end 
    assign counter = counter_up ^ text;

endmodule
