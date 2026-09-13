module example_top(
    input              clk,
    input              rst_n,
    input              load_secret,
    input              load_public,
    input              release_secret,
    input              allow_secret,
    input  [1:0] {1| |} mode,
    input  [7:0] {2| |} secret_bus,
    input  [7:0] {1| |} public_bus,
    output [7:0] {1| |} visible_result,
    output [7:0] {2| |} audit_result
);

wire [7:0] {1| |} filtered_public;
wire [7:0] {2| |} filtered_secret;
wire [7:0] {1| |} datapath_public;
wire [7:0] {2| |} datapath_audit;

reg [7:0] {1| |} visible_stage;
reg [7:0] {2| |} audit_stage;
reg [2:0] {1| |} valid_shift;

example_filter u_filter (
    .public_data(public_bus),
    .secret_data(secret_bus),
    .mode(mode),
    .allow_secret(allow_secret),
    .filtered_public(filtered_public),
    .filtered_secret(filtered_secret)
);

example_datapath u_datapath (
    .clk(clk),
    .rst_n(rst_n),
    .load_secret(load_secret),
    .load_public(load_public),
    .release_secret(release_secret),
    .secret_in(filtered_secret),
    .public_in(filtered_public),
    .public_out(datapath_public),
    .audit_out(datapath_audit)
);

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        visible_stage <= 8'h00;
        audit_stage <= 8'h00;
        valid_shift <= 3'b000;
    end else begin
        valid_shift <= {valid_shift[1:0], load_public | load_secret};

        if (valid_shift[2]) begin
            visible_stage <= datapath_public;
            audit_stage <= datapath_audit;
        end else begin
            visible_stage <= public_bus;
            audit_stage <= filtered_secret;
        end
    end
end

assign visible_result = visible_stage;
assign audit_result = audit_stage;

endmodule
