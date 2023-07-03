FROM ubuntu:18.04

RUN useradd -ms /bin/bash -u 1000 service

RUN apt update && apt install -y --no-install-recommends \
	nmap gcc make libc6-dev libc6 libc-dev-bin

COPY entrypoint.sh /
RUN chmod 755 /entrypoint.sh

COPY src/ /service
RUN make -C /service clean && make -C /service
RUN chmod +x /service/granulizer
RUN chown -R service:service /service
WORKDIR /service

EXPOSE 4321

# Run the service
#ENTRYPOINT ["/entrypoint.sh"]

ENTRYPOINT ["sh", "-c", "chmod -R 777 /service && su -c /entrypoint.sh service"]

#ENTRYPOINT ["sh", "-c", "chmod -R 777 /service && su -c /entrypoint.sh service"]
