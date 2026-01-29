(
~faustTests = (
	allTests: [
		\testBasic,
		\testNoScript,
		\testParameter,
		\testFaustLib,
		\testExcessiveOutputs,
		\testTooFewOutputs,
		\testWrongSyntax,
		\testFree,
		\testFreeAll,
	],

	testBasic: {
		var success = false;
		var condition = Condition();
		FaustDef(\testBasic, "process = 0.5;").send;
		s.sync;
		{
			Faust.ar(1, \testBasic);
		}.loadToFloatArray(0.1, action: {|sig|
			success = (sig[0]-0.5).abs <= 0.01;
			condition.unhang;
		});
		condition.hang;
		success;
	},

	testNoScript: {
		var success = false;
		// an unregistered script fails on a language level
		try {
			{Faust.ar(1, \noScript)}.loadToFloatArray(0.1);
		} {|error|
			success = true;
		};
		success;
	},

	testParameter: {
		var success = false;
		var condition = Condition();
		FaustDef(\testParam, "process = hslider(\"foo\",0,0,1,0.1);").send;
		s.sync;
		{
			Faust.ar(1, \testParam, params: [\foo, DC.ar(pi)]);
		}.loadToFloatArray(0.1, action: {|sig|
			success = (sig[0]-pi).abs <= 0.01;
			condition.unhang;
		});
		condition.hang;
		success;
	},

	testFaustLib: {
		var success = false;
		var condition = Condition();
		FaustDef(
			\testLib,
			"import(\"stdfaust.lib\"); process = os.triangle(300);"
		).send;
		s.sync;
		{
			Faust.ar(1, \testLib);
		}.loadToFloatArray(0.1, action: {|sig|
			success = sig[2] < 0.0;
			condition.unhang;
		});
		condition.hang;
		success;
	},

	testExcessiveOutputs: {
		var success = false;
		var condition = Condition();
		FaustDef(\testExcessive, "process = 0.5;").send;
		s.sync;
		{
			Faust.ar(2, \testExcessive);
		}.loadToFloatArray(0.1, action: {|sig|
			success = sig[0] == 0.0;
			condition.unhang;
		});
		condition.hang;
		success;
	},

	testTooFewOutputs: {
		var success = false;
		var condition = Condition();
		FaustDef(\testTooFew, "process = 0.5,0.6;").send;
		s.sync;
		{
			Faust.ar(1, \testTooFew);
		}.loadToFloatArray(0.1, action: {|sig|
			success = sig[0] == 0.0;
			condition.unhang;
		});
		condition.hang;
		success;
	},

	testWrongSyntax: {
		var success = false;
		var def = FaustDef(\x, "process = ").send;
		s.sync;
		success = def.params.size == 0;
		success;
	},

	testFree: {
		var playSuccess = false;
		var deleteSuccess = false;
		var langSuccess = false;

		var condition = Condition();

		var script = FaustDef(\testFree, "process = 0.5;").send;
		s.sync;
		{
			Faust.ar(1, script);
		}.loadToFloatArray(0.1, action: {|sig|
			playSuccess = (sig[0]-0.5).abs < 0.01;
			condition.unhang;
		});
		condition.hang;

		script.free;
		s.sync;
		langSuccess = FaustDef.all.keys.includes(\testFree).not;

		s.sync;
		{
			Faust.ar(1, script);
		}.loadToFloatArray(0.1, action: {|sig|
			deleteSuccess = (sig[0]-0.0).abs < 0.01;
			condition.unhang;
		});
		condition.hang;

		playSuccess.and(deleteSuccess).and(langSuccess);
	},

	testFreeAll: {
		var playSuccess = false;
		var deleteSuccess = false;
		var langSuccess = false;

		var condition = Condition();

		var script = FaustDef(\testFree, "process = 0.5;").send;
		s.sync;
		{
			Faust.ar(1, script);
		}.loadToFloatArray(0.1, action: {|sig|
			playSuccess = (sig[0]-0.5).abs < 0.01;
			condition.unhang;
		});
		condition.hang;

		FaustDef.freeAll;
		s.sync;
		langSuccess = FaustDef.all.keys.includes(\testFree).not;

		s.sync;
		{
			Faust.ar(1, script);
		}.loadToFloatArray(0.1, action: {|sig|
			deleteSuccess = (sig[0]-0.0).abs < 0.01;
			condition.unhang;
		});
		condition.hang;

		playSuccess.and(deleteSuccess).and(langSuccess);
	},

	// meta

	run: {|self, name=nil|
		var testsToRun = if(name.isNil, {self.allTests}, {name.asArray});
		var results = testsToRun.collect({|testName|
			"Running %".format(testName).postln;
			[testName, self.perform(testName)];
		});
		if(results.any(_[1].not), {
			"ERROR: There were errors".postln;
			results.select(_[1].not).do({|test|
				"ERROR: Test % failed!".format(test[0]).postln;
			});
		}, {
			"All tests ran successfully!".postln;
		});

	},
)
)

s.reboot;

fork{~faustTests.run};
