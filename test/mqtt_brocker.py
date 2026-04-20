import asyncio
import logging
from amqtt.broker import Broker

# 1. Improved logging configuration
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('broker')

# 2. Updated configuration (simplified for amqtt)
config = {
    'listeners': {
        'default': {
            'type': 'tcp',
            'bind': '0.0.0.0:1883',
        },
    },
    'sys_interval': 10,
    'auth': {
        'allow-anonymous': True,
    }
}

async def start_broker():
    broker = Broker(config)
    try:
        await broker.start()
        logger.info("Broker started and listening on 0.0.0.0:1883")
        # Keep the broker running
        while True:
            await asyncio.sleep(1)
    except asyncio.CancelledError:
        logger.info("Broker stopping...")
    finally:
        await broker.shutdown()

if __name__ == '__main__':
    try:
        # 3. Use asyncio.run() for modern Python versions
        asyncio.run(start_broker())
    except KeyboardInterrupt:
        logger.info("Broker stopped by user")
