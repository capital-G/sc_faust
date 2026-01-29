FaustDef {
	classvar <all;
	var <name;
	var <hash;
	var <>code;
	var <>params;

	// private
	classvar counter;
	// allows to obtain an object from a given hash
	// which is necessary for callbacks from the server
	classvar <>prHashMap;
	classvar recvFunc;

	*initClass {
		all = IdentityDictionary();
		prHashMap = IdentityDictionary();
		counter = 0;

		// this function receives the parsed parameters from the server
		// since it is difficult to send string data from the server
		// to the client, the content is shared via a file.
		// The client creates a file and sends its path to the server,
		// and the server will write the parsed parameters into this
		// file. After the server has written the content, the language
		// will be notified via a doneMsg, which will trigger this
		// function.
		recvFunc = {|msg, time, replyAddr, recvPort|
			if(msg.size == 2, {
				var doneMsg = msg[1].asString;
				if(doneMsg.beginsWith("faust"), {
					var paramString;
					var params;
					var hash = doneMsg.replace("faust", "").asInteger;
					var res = FaustDef.prHashMap[hash];
					var paramPath = FaustDef.prParamPath(hash);
					if(File.exists(paramPath).not, {
						"FaustGen % was not successfully compiled".format(res.name).warn;
					}, {
						paramString = File.readAllString(paramPath);
						params = paramString.split($$);
						// remove last empty param
						params = params.keep(params.size-1);
						// convert to symbol for lookup
						res.params = params.collect({|p| p.asSymbol});

						//"Set faust params for %".format(res).postln;
						File.delete(paramPath);
					});
				});
			});
		};
		thisProcess.addOSCRecvFunc(recvFunc);
	}

	*new {|name, code|
		var res;
		var hash;

		name = name.asSymbol;
		res = all[name];
		if(res.notNil, {
			if(code.isNil.not, {
				res.code = code;
			});
			^res;
		});
		hash = FaustDef.prHashSymbol(name);
		res = super.newCopyArgs(name, hash, code ? "");
		res.params = [];
		all[name] = res;
		FaustDef.prHashMap[hash] = res;
		^res;
	}

	*load {|name, path|
		var code = File.readAllString(path.asAbsolutePath);
		^this.new(name, code);
	}

	load {|path|
		code = File.readAllString(path.asAbsolutePath);
	}

	free {|server|
		server = server ? Server.default;
		server.sendMsg(*this.freeMsg);
		all[name] = nil;
	}

	freeMsg {
		^[
			\cmd,
			\faustfree,
			hash,
		];
	}

	*freeAll {|server|
		server = server ? Server.default;
		server.sendMsg(*this.freeAllMsg);
		all = [];
	}

	*freeAllMsg {
		^[
			\cmd,
			\faustfreeall,
		];
	}

	*defaultLibPath {
		^this.class.filenameSymbol.asString.dirname.dirname +/+ "externals" +/+ "faustlibs"
	}

	*libPath {|path, server|
		path = path ? this.defaultLibPath;
		server = server ? Server.default;
		"Set Faust Library path to %".format(path).postln;
		server.sendMsg(*this.libPathMsg(path));
	}

	*libPathMsg {|path|
		^[
			\cmd,
			\faustlibpath,
			path.asString,
		]
	}

	send {|server, completionMsg|
		var servers = (server ?? { Server.allBootedServers }).asArray;
		servers.do { |each|
			if(each.hasBooted.not) {
				"Server % not running, could not send FaustDef.".format(server.name).warn
			};
			this.prSendScript(each, completionMsg);
		}
	}

	prSendScript {|server, completionMsg|
		var tempPath = FaustDef.prParamPath(hash);
		var message = [\cmd, \faustscript, hash, tempPath, code, completionMsg].asRawOSC;
		if(message.size < (65535 div: 4), {
			server.sendRaw(message);
		}, {
			this.prSendFile(server, completionMsg);
		});
	}

	prSendFile {|server, completionMsg|
		var tmpFilePath = PathName.tmp +/+ "%_%".format(hash.asString, counter);
		counter = counter + 1;

		if (server.isLocal.not, {
			"FaustDef % could not be added  to server % because it is too big for sending via OSC and server is not local".format(
				name,
				server,
			).warn;
			^this;
		});

		File.use(tmpFilePath, "w", {|f|
			f.write(code);
		});

		server.sendMsg(\cmd, \faustfile, hash, tmpFilePath, completionMsg);

		fork {
			var deleteSuccess;
			server.sync;
			deleteSuccess = File.delete(tmpFilePath);
			if (deleteSuccess.not, {
				"Could not delete temp file % of FaustDef %".format(tmpFilePath, name).warn;
			});
		}
	}

	// takes an array of [\paramName, signal] which should be
	// transformed to its numerical representation of the `prParameters`
	// array. Non existing parameters will be thrown away,
	// also only the first occurence of a parameter will be considered
	prTranslateParameters {|parameters|
		var newParameters = [];
		parameters.pairsDo({|param, value|
			var index = params.indexOf(param.asSymbol);
			if(index.notNil, {
				newParameters = newParameters.add(index).add(value);
			}, {
				"Parameter % is not registered in % - will be ignored".format(
					param,
					name,
				).warn;
			});
		});
		^newParameters;
	}

	*prHashSymbol {|symbol|
		// hash numbers are too high to represent as a float32
		// on the server, so we have to scale those down.
		// 2**20 seems okayish?
		^(symbol.hash.abs % (2**20) * symbol.hash.sign).asInteger;
	}

	*prParamPath {|hash|
		^Platform.defaultTempDir +/+ "faust%".format(hash);
	}
}


Faust : MultiOutUGen {
	*ar {|numOutputs, script, inputs, params|
		var signals;

		script = case
		{script.isKindOf(FaustDef)} {script}
		{script.isKindOf(String)} {FaustDef.all[script.asSymbol]}
		{script.isKindOf(Symbol)} {FaustDef.all[script.asSymbol]}
		{Error("Script input needs to be a FaustDef object or a symbol, found %".format(script.class)).throw};
		if(script.isNil, {
			Error("Could not find script").throw;
		});

		inputs = inputs.asArray;
		params = params.asArray;

		if(params.size.odd, {
			Error("Parameters need to be key-value pairs, but found an odd number of elements").throw;
		});

		signals = inputs ++ params;

		^this.multiNew(
			'audio',
			numOutputs,
			script,
			inputs.size,
			params.size/2, // parameters are tuples of [id, value]
			*signals,
		);
	}

	init { |numOutputs, script, numInputs, numParams ... signals|
		var params = signals[numInputs..];
		var audioInputs = signals[..(numInputs-1)];

		params.pairsDo({|key, value|
			if(key.isKindOf(String).or(key.isKindOf(Symbol)).not, {
				Error("'%' is not a valid parameter key".format(key)).throw;
			});
			if(value.isValidUGenInput.not, {
				Error("Parameter '%' has invalid value '%'".format(key, value)).throw;
			});
		});

		params = script.prTranslateParameters(params);
		// update the parameter count because parameters might have been ignored!
		numParams = params.size.div(2);

		// signals must be audio rate
		signals = audioInputs.collect({|sig|
			if(sig.rate != \audio, {
				K2A.ar(sig);
			}, {
				sig;
			});
		});

		params.pairsDo({|index, value|
			// parameter values must be audio rate,
			if(value.rate != \audio, {
				value = K2A.ar(value);
			});
			signals = signals.add(index).add(value);
		});

		// inputs is a member variable of UGen
		inputs = [
			script.hash.asFloat,
			numInputs,
			numParams,
		] ++ signals;

		^this.initOutputs(numOutputs, \audio);
	}
}
