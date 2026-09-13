module example_datapath(
    input              clk,
    input              rst_n,
    input              load_secret,
    input              load_public,
    input              release_secret,
    input  [7:0] {2| |} secret_in,
    input  [7:0] {1| |} public_in,
    output reg [7:0] {1| |} public_out,
    output reg [7:0] {2| |} audit_out
);

reg [7:0] {2| |} secret_reg;
reg [7:0] {1| |} public_reg;
reg [7:0] {2| |} mixed_reg;
reg [3:0] {1| |} round_ctr;
reg             busy;

wire [7:0] {1| |} public_next;
wire [7:0] {2| |} secret_next;

assign public_next = public_reg + {4'h0, round_ctr};
assign secret_next = secret_reg ^ mixed_reg;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        secret_reg <= 8'h00;
        public_reg <= 8'h00;
        mixed_reg <= 8'h00;
        public_out <= 8'h00;
        audit_out <= 8'h00;
        round_ctr <= 4'h0;
        busy <= 1'b0;
    end else begin
        if (load_secret) begin
            secret_reg <= secret_in;
            mixed_reg <= secret_in ^ public_reg;
            busy <= 1'b1;
        end

        if (load_public) begin
            public_reg <= public_in;
            mixed_reg <= secret_reg ^ public_in;
            busy <= 1'b1;
        end

        if (busy) begin
            round_ctr <= round_ctr + 4'h1;
            audit_out <= secret_next;
        end

        if (release_secret) begin
            public_out <= secret_reg;
        end else begin
            public_out <= public_next;
        end

        if (round_ctr == 4'hf) begin
            busy <= 1'b0;
        end
    end
end

endmodule
