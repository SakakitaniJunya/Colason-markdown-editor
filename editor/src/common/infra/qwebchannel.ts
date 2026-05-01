/**
 * QWebChannel - TypeScript implementation for Qt6 WebChannel communication.
 * Based on Qt6's qwebchannel.js (LGPLv3 / commercial licensed by The Qt Company).
 * Minimal implementation supporting object registration, signals, and method calls.
 */

type QWebChannelTransport = {
  send(data: string): void;
  onmessage?: (message: { data: string }) => void;
};

type QWebChannelCallback = (channel: QWebChannel) => void;

enum QWebChannelMessageTypes {
  signal = 1,
  propertyUpdate = 2,
  init = 3,
  idle = 4,
  debug = 5,
  invokeMethod = 6,
  connectToSignal = 7,
  disconnectFromSignal = 8,
  setProperty = 9,
  response = 10,
}

export class QWebChannel {
  transport: QWebChannelTransport;
  objects: Record<string, QObject> = {};
  private execCallbacks: Record<number, (data: any) => void> = {};
  private execId = 0;

  constructor(transport: QWebChannelTransport, initCallback?: QWebChannelCallback) {
    this.transport = transport;
    this.transport.onmessage = (message) => {
      const data = typeof message.data === 'string' ? JSON.parse(message.data) : message.data;
      this.handleMessage(data);
    };

    this.exec({ type: QWebChannelMessageTypes.init }, (data: any) => {
      for (const objectName in data) {
        this.objects[objectName] = new QObject(objectName, data[objectName], this);
      }
      // now unwrap properties, which might reference other registered objects
      for (const objectName in this.objects) {
        this.objects[objectName].unwrapProperties();
      }
      initCallback?.(this);
      this.exec({ type: QWebChannelMessageTypes.idle });
    });
  }

  send(data: any): void {
    this.transport.send(typeof data === 'string' ? data : JSON.stringify(data));
  }

  exec(data: any, callback?: (data: any) => void): void {
    if (callback) {
      const id = this.execId++;
      this.execCallbacks[id] = callback;
      data.id = id;
    }
    this.send(data);
  }

  private handleMessage(message: any): void {
    switch (message.type) {
      case QWebChannelMessageTypes.signal:
        this.handleSignal(message);
        break;
      case QWebChannelMessageTypes.response:
        this.handleResponse(message);
        break;
      case QWebChannelMessageTypes.propertyUpdate:
        this.handlePropertyUpdate(message);
        break;
      default:
        console.warn('[QWebChannel] Unhandled message type:', message.type);
    }
  }

  private handleSignal(message: any): void {
    const object = this.objects[message.object];
    object?.signalEmitted(message.signal, message.args);
  }

  private handleResponse(message: any): void {
    if (message.id === undefined) return;
    const callback = this.execCallbacks[message.id];
    if (callback) {
      delete this.execCallbacks[message.id];
      callback(message.data);
    }
  }

  private handlePropertyUpdate(message: any): void {
    for (const data of message.data) {
      const object = this.objects[data.object];
      if (object) {
        object.propertyUpdate(data.signals, data.properties);
      }
    }
    this.exec({ type: QWebChannelMessageTypes.idle });
  }
}

class QObject {
  __id__: string;
  private webChannel: QWebChannel;
  private signalHandlers: Record<number, Array<(...args: any[]) => void>> = {};
  private methods: string[] = [];
  private properties: Record<string, any> = {};
  private enums: Record<string, any> = {};
  private objectSignals: Record<string, number> = {};
  private propertySignalMap: Record<number, string> = {};

  [key: string]: any;

  constructor(name: string, data: any, webChannel: QWebChannel) {
    this.__id__ = name;
    this.webChannel = webChannel;

    // methods
    this.methods = data.methods || [];
    for (const methodIdx of this.methods) {
      this.bindMethod(methodIdx);
    }

    // signals
    for (const signalName in data.signals || {}) {
      this.objectSignals[signalName] = data.signals[signalName];
      this.bindSignal(signalName, data.signals[signalName]);
    }

    // properties
    for (const propName in data.properties || {}) {
      const propInfo = data.properties[propName];
      this.bindProperty(propName, propInfo);
    }

    // enums
    for (const enumName in data.enums || {}) {
      this[enumName] = data.enums[enumName];
    }
  }

  signalEmitted(signalName: number, args: any[]): void {
    const handlers = this.signalHandlers[signalName];
    if (handlers) {
      for (const handler of handlers) {
        handler.apply(handler, args);
      }
    }
  }

  propertyUpdate(signals: number[], propertyMap: Record<string, any>): void {
    for (const propName in propertyMap) {
      this.properties[propName] = propertyMap[propName];
    }
    for (const signalName of signals) {
      this.signalEmitted(signalName, [this.properties[this.propertySignalMap[signalName]]]);
    }
  }

  unwrapProperties(): void {
    for (const propName in this.properties) {
      this.properties[propName] = this.unwrapQObject(this.properties[propName]);
    }
  }

  private unwrapQObject(response: any): any {
    if (response instanceof Array) {
      return response.map((item) => this.unwrapQObject(item));
    }
    if (!(response instanceof Object) || response.__QObject__ === undefined) {
      return response;
    }
    const objectId = response.id;
    if (this.webChannel.objects[objectId]) {
      return this.webChannel.objects[objectId];
    }
    if (!response.data) {
      console.error('[QWebChannel] Missing data for QObject:', objectId);
      return undefined;
    }
    const qObject = new QObject(objectId, response.data, this.webChannel);
    qObject.destroyed.connect(() => {
      if (this.webChannel.objects[objectId] === qObject) {
        delete this.webChannel.objects[objectId];
      }
    });
    qObject.unwrapProperties();
    return qObject;
  }

  private bindMethod(methodIdx: any): void {
    const methodName = typeof methodIdx === 'object' ? methodIdx[0] : methodIdx;
    const methodIndex = typeof methodIdx === 'object' ? methodIdx[1] : methodIdx;

    this[methodName] = (...args: any[]) => {
      const callArgs: any[] = [];
      let callback: ((result: any) => void) | undefined;

      for (const arg of args) {
        if (typeof arg === 'function') {
          callback = arg;
        } else {
          callArgs.push(arg);
        }
      }

      this.webChannel.exec(
        {
          type: QWebChannelMessageTypes.invokeMethod,
          object: this.__id__,
          method: methodIndex,
          args: callArgs,
        },
        (response: any) => {
          callback?.(this.unwrapQObject(response));
        }
      );
    };
  }

  private bindSignal(signalName: string, signalIndex: number): void {
    this[signalName] = {
      connect: (callback: (...args: any[]) => void) => {
        if (!this.signalHandlers[signalIndex]) {
          this.signalHandlers[signalIndex] = [];
        }
        this.signalHandlers[signalIndex].push(callback);

        // Tell C++ side we want to receive this signal
        if (this.signalHandlers[signalIndex].length === 1) {
          this.webChannel.exec({
            type: QWebChannelMessageTypes.connectToSignal,
            object: this.__id__,
            signal: signalIndex,
          });
        }
      },
      disconnect: (callback: (...args: any[]) => void) => {
        if (!this.signalHandlers[signalIndex]) return;
        const idx = this.signalHandlers[signalIndex].indexOf(callback);
        if (idx !== -1) {
          this.signalHandlers[signalIndex].splice(idx, 1);
        }
        if (this.signalHandlers[signalIndex].length === 0) {
          this.webChannel.exec({
            type: QWebChannelMessageTypes.disconnectFromSignal,
            object: this.__id__,
            signal: signalIndex,
          });
        }
      },
    };
  }

  private bindProperty(propName: string, propInfo: any): void {
    const propIndex = propInfo[0];
    const propValue = propInfo[1];
    const notifySignalIndex = propInfo[2];

    this.properties[propName] = propValue;
    this.propertySignalMap[notifySignalIndex] = propName;

    Object.defineProperty(this, propName, {
      configurable: true,
      get: () => this.properties[propName],
      set: (value: any) => {
        if (value === undefined) {
          console.warn('[QWebChannel] Property "' + propName + '" set to undefined');
          return;
        }
        this.properties[propName] = value;
        this.webChannel.exec({
          type: QWebChannelMessageTypes.setProperty,
          object: this.__id__,
          property: propIndex,
          value: value,
        });
      },
    });

    // Add notify signal connection
    if (notifySignalIndex !== undefined) {
      if (!(notifySignalIndex in this.objectSignals)) {
        // property change signals aren't listed in signals, create binding
        this.bindSignal('__prop_notify_' + propName, notifySignalIndex);
      }
    }
  }
}
