//=================================================================================================
//	REED
//=================================================================================================
vec4 reed_excitation(bool isClarinet)
{
	float k, h0, aw, mouthpiece_height;
	if (isClarinet) {
		// clarinet values:
		// instr.height: 0.01
		// k: 8e6
		// h0: 0.6e-3
		// aw: 12e-3
		k = 8e6f;
		h0 = 0.6e-3f;
		aw = 12e-3f;
		mouthpiece_height = 0.01f;
	}
	else {
		// sax values:
		// instr.height: 0.005
		// k: 13e6
		// h0: 1e-3
		// aw: 12e-3
		k = 11e6f;
		h0 = 0.8e-3f;
		aw = 12e-3;
		mouthpiece_height = 0.01f;
	}
	
	float p_bore = getFeedback(quad-numOfStates); // read pressure feedback [mapped to PREVIOUS FDTD STEP!];
	
	// Read previous outputs
	vec4 prev_out     = texture(inOutTexture, texture0); // N
	vec4 prev_out_old = texture(inOutTexture, texture2); // N-1
	
	float input_control = (excitationInput * 0.5) + 0.5;

	const float dp_max = h0 * k;
	const float inv_mouthpiece_height = 1.0f / mouthpiece_height;

	//const float p_mouth = dp_max * (((excitationInput * 0.5) + 0.5) * 0.6 + 0.4);
	//const float p_mouth = dp_max * 0.7;// *(((excitationInput * 0.5) + 0.5) * 0.6 + 0.4);
	//const float p_mouth = dp_max * ((excitationInput) * 0.6 + 0.2);
	//const float p_mouth = dp_max * ((excitationInput) * 0.7 + 0.25);
	const float p_mouth = dp_max * excitationInput * 0.1;

	float dp = p_mouth - p_bore; 

	float x = clamp(dp / dp_max, 0.02f, 1);

	float tanh_width = 0.005;	// half-maximum tanh @ x = width
	
	float to_left_tanh = clamp(3.8f / tanh_width * (x - tanh_width), -10, 10);
	float left_tanh =  (1 + tanh(to_left_tanh)) * 0.5f;

	float to_right_tanh = clamp(3.8f / tanh_width * (x - 1.0f + tanh_width), -10, 10);
	float right_tanh = (1 - tanh(to_right_tanh)) * 0.5f;

	float feathering = (1 - abs(x)) * sqrt(abs(x)) * left_tanh * right_tanh * sign(x);
	
	float u = aw * h0 * sqrt(2.0f * dp_max / rho) * feathering;

	float vout = (inv_ds * inv_mouthpiece_height) * u * inv_exCellNum;
	

	//-------------------------------------------------------------------
	// butterworth low pass second order filter
	//-------------------------------------------------------------------
	// current and previous excitation values, inputs of the filter
	float filter_x0 = vout;           // input at N+1
	float filter_x1 = prev_out.r;     // input at N
	float filter_x2 = prev_out_old.r; // input at N-1
	
	// previous filtered outputs
	float filter_y1 = texture(inOutTexture, texture1).a; // output at N  
	float filter_y2 = texture(inOutTexture, texture3).a; // output at N-1

	// current filtered output
	float filter_y0 = exLp_b0*filter_x0 + exLp_b1*filter_x1 + exLp_b2*filter_x2 - exLp_a1*filter_y1 - exLp_a2*filter_y2;
	//-------------------------------------------------------------------

	return vec4(vout, 0, 0, filter_y0);
}