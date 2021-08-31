import { createHash } from "crypto";
import { WebSocket } from "ws";

export async function sleep(millis) {
  await new Promise(resolve => {
    setTimeout(() => {
      resolve(undefined);
    }, millis);
  });
} 

export function md5(data) {
  const hash = createHash("md5");
  hash.update(data);
  return hash.digest("hex");
}

export class WebSocketReceiver {
  constructor(ws) {
    this.messages = [];
    this.pending = null;

    ws.on("open", () => {
      this.#log("open");
      this.#push({type: "open"});
    });
    ws.on("message", (data, isBinary) => {
      this.#log("message");
      this.#push({type: "message", data, isBinary});
    });
    ws.on("close", ({code, reason}) => {
      this.#log("close");
      this.#push({type: "close", code, reason});
    });
    ws.on("error", error => {
      this.#log("error");
      this.#push({type: "error", error});
    });
  }

  #log(...args) {
    // console.log(...args);
  }

  #push(message) {
    this.messages.push(message);
    if (this.messages.length === 1 && this.pending) {
      const prevPending = this.pending
      this.pending = null;
      prevPending.resolve(null);
    }
  }

  async nextEvent(type = "message") {
    while (this.messages.length === 0) {
      if (!this.pending) {
        let resolve, reject;
        let promise = new Promise((resolve_, reject_) => {
          resolve = resolve_;
          reject = reject_;
        });
        this.pending = {promise, resolve, reject};
      }
      await this.pending.promise;
    }
    const msg = this.messages.shift();
    if (type && msg.type !== type) {
      if (msg.type === "error") {
        throw msg.error;
      } else {
        throw new Error(`Unexpected WebSocket event (expected '${type}', got '${msg.type}')`);
      }
    }
    return msg;
  }
}

export async function withWS(address, options, callback) {
  console.log("Connecting to", address, "with", options);
  const ws = new WebSocket(address, options);
  try {
    const wsr = new WebSocketReceiver(ws);
    await wsr.nextEvent("open");

    return await callback(ws, wsr);
  } finally {
    ws.close();
  }
}
