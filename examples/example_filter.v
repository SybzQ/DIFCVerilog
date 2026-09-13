module example_filter(
    input  [7:0] {1| |} public_data,
    input  [7:0] {2| |} secret_data,
    input  [1:0] {1| |} mode,
    input              allow_secret,
    output reg [7:0] {1| |} filtered_public,
    output reg [7:0] {2| |} filtered_secret
);

always @* begin
    filtered_public = public_data;
    filtered_secret = secret_data;

    case (mode)
        2'b00: begin
            filtered_public = public_data;
            filtered_secret = secret_data;
        end

        2'b01: begin
            filtered_public = public_data ^ 8'h3c;
            filtered_secret = secret_data;
        end

        2'b10: begin
            filtered_public = public_data + 8'h11;
            filtered_secret = secret_data ^ public_data;
        end

        default: begin
            filtered_public = allow_secret ? secret_data : public_data;
            filtered_secret = allow_secret ? secret_data : 8'h00;
        end
    endcase
end

endmodule
