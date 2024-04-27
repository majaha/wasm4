// Created using `npm run build:apu-worklet` and
// is automatically generated in build and start scripts.
import workletRawSource from "./apu-worklet.min.generated.js?raw";

declare global {
    interface Window {
      webkitAudioContext: typeof AudioContext
    }
}

export class APU {
    audioCtx: AudioContext;
    processor!: APUProcessor;
    processorPort!: MessagePort;
    bufferedToneCalls: BufferedToneCalls = [null, null, null, null];

    constructor () {
        this.audioCtx = new (window.AudioContext || window.webkitAudioContext)({
            sampleRate: 44100, // must match SAMPLE_RATE in worklet
        });
    }

    async init () {
        const audioCtx = this.audioCtx;
        const blob = new Blob([workletRawSource], {type: "application/javascript"});
        const url = URL.createObjectURL(blob);

        try {
            await audioCtx.audioWorklet.addModule(url);

            const workletNode = new AudioWorkletNode(audioCtx, "wasm4-apu", {
                outputChannelCount: [2],
            });
            this.processorPort = workletNode.port;
            workletNode.connect(audioCtx.destination);

        } catch (error) {
            console.warn("AudioWorklet loading failed, falling back to slow audio", error);

            // Scoop out the APUProcessor with a simple polyfill
            let processor!: APUProcessor;
            const registerProcessor: typeof globalThis.registerProcessor = (name, p) => {
                processor = new p() as APUProcessor;
            }
            const fn = new Function("registerProcessor", "AudioWorkletProcessor", workletRawSource);
            fn(registerProcessor, class {});
            this.processor = processor;

            const scriptNode = audioCtx.createScriptProcessor(1024, 0, 2);
            scriptNode.onaudioprocess = event => {
                const outputLeft = event.outputBuffer.getChannelData(0);
                const outputRight = event.outputBuffer.getChannelData(1);
                processor.process(null, [[outputLeft, outputRight]], null);
            };
            scriptNode.connect(audioCtx.destination);
        }
    }

    tick () {
        if (this.processorPort != null) {
            // Send the buffered tone calls for this tick to the audio worklet
            this.processorPort.postMessage(this.bufferedToneCalls);
        } else {
            // For the ScriptProcessorNode fallback, just call tick() directly
            this.processor.tick(this.bufferedToneCalls);
        }
        // Clear the buffered calls for the next update
        this.bufferedToneCalls = [null, null, null, null];
    }

    tone (frequency: number, duration: number, volume: number, flags: number) {
        const channelIdx = flags & 0x3;

        this.bufferedToneCalls[channelIdx] = [frequency, duration, volume, flags];
    }

    unlockAudio () {
        const audioCtx = this.audioCtx;
        if (audioCtx.state == "suspended") {
            audioCtx.resume();
        }
    }

    pauseAudio () {
        const audioCtx = this.audioCtx;
        if (audioCtx.state == "running") {
            audioCtx.suspend();
        }
    }
}
