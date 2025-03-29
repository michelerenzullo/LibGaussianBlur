const waitFor = condition => {
	// recursively set a timeout till condition has been satisfied
	const poll = resolve => condition() ? resolve() : setTimeout(() => poll(resolve), 5)
	return new Promise(poll)
}

onmessage = async e => {
	const file = e.data[0]
	const arguments = e.data[1]

	const startTime = performance.now()
	const Module = h //needed because of closure

	//js string to C like const char array with null terminator
	const utf8 = new Uint8Array(arguments.length + 1);
	const stats = new TextEncoder().encodeInto(arguments, utf8)
	utf8[arguments.length] = 0

	// check that native functions can be safely called 
	// otherwise await runtime initialization (runtimeInitialized === true)
	// Note: when closure compiling var runtimeInitialized is optimized away
	// so check that Module._malloc !== undefined
	await waitFor(() => Module._malloc !== undefined /* runtimeInitialized === true */);

	const args_ptr = Module._malloc(utf8.byteLength)
	Module.HEAPU8.set(utf8, args_ptr)

	const target_ptr = Module._malloc(file.byteLength)
	Module.HEAPU8.set(file, target_ptr)

	//ptr will be free internally in C++
	const result = Module.start(args_ptr, target_ptr, file.byteLength)

	//benchmark enabled
	const time = performance.now() - startTime
	console.log("WebWorker JS time: " + time + " ms")
	postMessage([result, time])
	//postMessage([result])
}

importScripts('GaussianBlur.js')