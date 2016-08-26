
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*
 * nas_nlmsgl.h
 *
 *  Created on: Mar 25, 2015

 */

#ifndef CPS_API_LINUX_INC_PRIVATE_NAS_NLMSG_H_
#define CPS_API_LINUX_INC_PRIVATE_NAS_NLMSG_H_

#include <linux/rtnetlink.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int nlmsg_msg_size(int payload) {
    return NLMSG_HDRLEN + payload;
}

static inline int nlmsg_total_size(int payload) {
    return NLMSG_ALIGN(nlmsg_msg_size(payload));
}

static inline void *nlmsg_data(const struct nlmsghdr *nlh) {
    return (void*) NLMSG_DATA(nlh);
}

static inline struct nlattr *nlmsg_attrdata(const struct nlmsghdr *nlh,
                                            int nlmsg_hdr_len) {
    unsigned char *data = (unsigned char*)nlmsg_data(nlh);
    return (struct nlattr *) (data + NLMSG_ALIGN(nlmsg_hdr_len));
}

static inline int nlmsg_len(const struct nlmsghdr *nlh) {
    return nlh->nlmsg_len - NLMSG_HDRLEN;
}

static inline int nlmsg_attrlen(const struct nlmsghdr *nlh, int nlmsg_hdr_len) {
    return nlmsg_len(nlh) - NLMSG_ALIGN(nlmsg_hdr_len);
}

/**
 * This API will take a netlink message header and return the next netlink message header and update
 * remaining accordingly
 * @param nlh is the current netlink header
 * @param remaining is the remaining length
 * @return poiner to the next netlink message
 */
static inline struct nlmsghdr * nlmsg_next(const struct nlmsghdr *nlh, int *remaining) {
    int totlen = NLMSG_ALIGN(nlh->nlmsg_len);
    *remaining -= totlen;
    return (struct nlmsghdr *) ((unsigned char *) nlh + totlen);
}

/**
 * Verify that the netlink message fits within the length specified
 * @param nlh the existing netlink message
 * @param len the length of the netlink message buffer
 * @return true if will fit
 */
static inline bool nlmsg_ok(struct nlmsghdr *nlh, unsigned int len) {
    return ((len) >= (int)sizeof(struct nlmsghdr) &&
                             (nlh)->nlmsg_len >= sizeof(struct nlmsghdr) && \
                             (nlh)->nlmsg_len <= (len));
}

typedef void (*netlink_tool_msg_iterator)(struct nlmsghdr *msg, void *context);

/**
 * This API will walk over each netlink message in a message buffer
 * @param head start of the list of netlink headers
 * @param len of buffer
 * @param fun the function iterator that will be called for each message
 * @param context the applicaiton specific context that is used for each call
 */
static inline void nlmsg_for_each_msg(struct nlmsghdr * head, int len,
        netlink_tool_msg_iterator fun, void * context)  {
    struct nlmsghdr *pos = head;
    int rem = len;
    for ( ; nlmsg_ok(pos, rem); pos = nlmsg_next(pos, &(rem))) {
        fun(pos,context);
    }
}

/**
 * Get the next netlink attribute
 * @param nla the current netlink attribute
 * @param remaining[out] the remaining space after this call (will be updated)
 * @return the next netlink attribute
 */
static inline struct nlattr *nla_next(const struct nlattr *nla, int *remaining) {
    int totlen = NLA_ALIGN(nla->nla_len);
    *remaining -= totlen;
    return (struct nlattr *) ((char *) nla + totlen);
}

/**
 * Verify that the netlink attribute is ok - valid within the size constraints
 * @param nla the attribute
 * @param remaining space remaining
 * @return 0 if not valid otherwise non-zero
 */
static inline int nla_ok(const struct nlattr *nla, int remaining) {
    return remaining >= (int) sizeof(*nla) &&
           nla->nla_len >= sizeof(*nla) &&
           nla->nla_len <= remaining;
}

/**
 * nla_for_each_attr - iterate over a stream of attributes
 * @pos: loop counter, set to current attribute
 * @head: head of attribute stream
 * @len: length of attribute stream
 * @rem: initialized to len, holds bytes currently remaining in stream
 */

typedef void (*netlink_tool_attr_iterator)(struct nlattr *attr, void *context);
/**
 * Iterate over each netlink attribute in the bufer updating remaining as it goes using len to start
 * @param head is the current head
 * @param len is the total length of all attributes in the buffer
 * @param fun is the function call that will be called for each attribute
 * @param context is the context that will be passed to the fun as the second param
 */
static inline void nla_for_each_attr(struct nlattr *head, int len,
        netlink_tool_attr_iterator fun, void * context)  {
    struct nlattr *pos = head;
    int rem =len;
    for ( ; nla_ok(pos, rem); pos = nla_next(pos, &(rem))) {
        fun(pos,context);
    }
}

/**
 * Get the netlink attribute's data
 * @param nla the netlink attribute pointer
 */
static inline void *nla_data(const struct nlattr *nla) {
    return ((char *) nla) + NLA_HDRLEN;
}

/**
 * Get the netlink attribute type
 * @param nla netlink attribute type
 * @return just the netlink attr type
 */
static inline int nla_type(const struct nlattr *nla) {
    return nla->nla_type & NLA_TYPE_MASK;
}

/**
 * Get the length of the netlink attribute
 * @param nla netlink attribute
 * @return length of data
 */
static inline int nla_len(const struct nlattr *nla) {
    return nla->nla_len - NLA_HDRLEN;
}

/**
 * For each netlink attribute contained by a netlink attribute - walk the list
 * @param head is the current netlink attribute
 * @param len is a variable that will keep track of the remaining data length
 * @param fun is the function call that will be called for each attribute
 * @param context is the context that will be passed to the fun as the second param
 */
static inline void nla_for_each_emb_attr(struct nlattr *head, int len,
        netlink_tool_attr_iterator fun, void * context) {
    nla_for_each_attr((struct nlattr*)nla_data(head),nla_len(head), fun,context);
}


/**
 * Parse all of the netlink attributes in a netlink message starting at the head.  Return
 * in a array of nlattr (s)
 * @param tb is the array of netlink attributes
 * @param max_type is the total size of the netlink attrib list
 * @param head is the start of the netlink attributes
 * @param len is the length of the buffer to process
 * @return 0 if successful otherwise non zero
 */
int nla_parse(struct nlattr *tb[], int max_type, struct nlattr * head, int len) ;

/**
 * Parse a nested attribute structure using the nla_parse function
 * @param tb is the array returned
 * @param max_type the length of the array
 * @param nla is the attribute with the embedded attributes to parse
 * @return 0 if successful otherwise non-zero
 */
static inline int nla_parse_nested(struct nlattr *tb[], int max_type,
                                    struct nlattr *nla ){
    return nla_parse(tb, max_type, (struct nlattr *)nla_data(nla), nla_len(nla));
}

static inline struct rtnexthop* rtnh_next(struct rtnexthop * rtnh, int *len) {
    *len -= RTNH_ALIGN(rtnh->rtnh_len);
    return RTNH_NEXT(rtnh);
}

static inline struct rtattr* rtnh_data(struct rtnexthop * rtnh) {
    return RTNH_DATA(rtnh);
}

static inline int rtnh_attr_len(struct rtnexthop * rtnh) {
    return rtnh->rtnh_len - RTNH_LENGTH(0);
}

static inline void nhrt_parse(struct nlattr *tb[], int max_type, struct rtnexthop * rtnh ) {
    nla_parse(tb,__IFLA_MAX,
                    (struct nlattr*)RTNH_DATA(rtnh),rtnh_attr_len(rtnh));
}

static inline void* nlmsg_tail(struct nlmsghdr * m) {
    return (void *) (((char*)m) + NLMSG_ALIGN(m->nlmsg_len));
}

void * nlmsg_reserve(struct nlmsghdr * m, int maxlen, int len);
int nlmsg_add_attr(struct nlmsghdr * m, int maxlen, int type, const void * data, int attr_len);

struct nlattr * nlmsg_nested_start(struct nlmsghdr * m, int maxlen);
void nlmsg_nested_end(struct nlmsghdr * m, struct nlattr *attr);


#ifdef __cplusplus
}
#endif

#endif /* CPS_API_LINUX_INC_PRIVATE_NAS_NLMSG_H_ */
